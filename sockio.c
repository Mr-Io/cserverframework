#include "sockio.h"
#include "exerror.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>

#define MAXBUFSIZE 8192
#define SOMAXCONN 4096
#define IPLEN 128
#define PORTLEN 32


typedef struct TCPSOCKET {
    size_t nleft;
    int fd;
    int ungotten;   /* ungetc char */
    char *ip_conn;
    char *port_conn;
    char *bufp;
    char buf[MAXBUFSIZE];
}TCPSOCKET;

/* explicitly threat-safe */
static TCPSOCKET *init_tcpsocket(int fd) 
{
    TCPSOCKET *res;
    res = (TCPSOCKET *)Malloc(sizeof(TCPSOCKET));
    res->fd = fd;
    res->nleft = 0;
    res->bufp = res->buf;
    res->ungotten = EOF;
    res->ip_conn = res->port_conn = NULL;
    return res;
}

/* implicitly thread-safe */
int tcpsocket_close(TCPSOCKET *sstream)
{
    if (sstream->nleft > 0){
        fprintf(stderr, "warning: tcpsocket still has %lu bytes unread\n", sstream->nleft);
    }
    Close(sstream->fd);
    if (sstream->ip_conn){
        free(sstream->ip_conn);
    }
    if (sstream->port_conn){
        free(sstream->port_conn);
    }
    free(sstream);
    return 0;
}

/* implicitly thread-safe */
TCPSOCKET *tcpsocket_connect(const char hostname[], const char port[])
{    
    struct addrinfo hints; 
    struct addrinfo *res;

    memset(&hints, 0, sizeof hints);
    hints.ai_flags = AI_ADDRCONFIG;
    hints.ai_socktype = SOCK_STREAM;

    int errcode;
    while((errcode = getaddrinfo(hostname, port, &hints, &res)) != 0){
        if (errcode != EAI_AGAIN){
            fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(errcode));
            return NULL; 
        }
    };

    struct addrinfo *res_save = res;
    int fd;
    int connected;
    while (res != NULL){
        if((fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) == -1){
            continue;
        }
        do{
            connected = connect(fd, res->ai_addr, res->ai_addrlen);
        }while(connected != 0 && errno == EINTR);
        if (connected == 0){
            break;
        } 
        Close(fd);
        res = res->ai_next;
    }
    freeaddrinfo(res_save);

    if (!res){
        return NULL;
    }
    return init_tcpsocket(fd);
}

/* implicitly thread-safe */
static int tcplisten(const char *port, int ai_family)
{
    struct addrinfo hints;
    struct addrinfo *res;

    memset(&hints, 0, sizeof hints);
    hints.ai_flags = AI_PASSIVE | AI_ADDRCONFIG;
    hints.ai_family = ai_family;
    hints.ai_socktype = SOCK_STREAM;

    int errcode;
    if ((errcode = getaddrinfo(NULL, port, &hints, &res)) != 0){
        return errcode;
    };

    /* check that there is only one possible address */
    if(res->ai_next != NULL){
        return -1;
    }

    /* create and bind socket */
    struct addrinfo *save_res = res;
    int listenfd;
    listenfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (listenfd != -1){
        errcode = bind(listenfd, res->ai_addr, res->ai_addrlen);
    }
    freeaddrinfo(save_res);
    if (errcode == -1){
        Close(listenfd); 
        return -1;
    }
    if (listen(listenfd, SOMAXCONN) == -1){
        fprintf(stderr, "listen error: %s\n", strerror(errno));
        Close(listenfd);
        return -1;
    }
    return listenfd;
}

/* implicitly thread-safe */
int tcplisten_ipv4(const char *port)
{
    return tcplisten(port, AF_INET);
}

/* implicitly thread-safe */
int tcplisten_ipv6(const char *port)
{
    return tcplisten(port, AF_INET6);
}

/* explicitly thread-safe */
TCPSOCKET *tcpsocket_accept(int listenfd)
{
    struct sockaddr_storage generic_addr;
    unsigned int generic_len;
    int socketfd;
    generic_len = sizeof generic_addr;
    while((socketfd = accept(listenfd, (struct sockaddr *) &generic_addr, &generic_len)) == -1){
        if (errno != EINTR){
            fprintf(stderr, "accept error:%s\n", strerror(errno));
            return NULL;
        }
    }
    TCPSOCKET *sstream = init_tcpsocket(socketfd);
    int flags = NI_NUMERICHOST | NI_NUMERICSERV;
    sstream->ip_conn = malloc(IPLEN * sizeof(char));
    sstream->port_conn = malloc(PORTLEN * sizeof(char));
    int errcode;
    do{
        errcode = getnameinfo(
                (struct sockaddr *) &generic_addr, generic_len,
                sstream->ip_conn, IPLEN,
                sstream->port_conn, PORTLEN, flags);
    }while(errcode != 0 && errcode == EAI_AGAIN);
    if (errcode != 0){
        fprintf(stderr, "getnameinfo error: %s\n", gai_strerror(errcode));
        tcpsocket_close(sstream);
        return NULL; 
    }
    return sstream;
}

/* implicitly thread-safe */
char *get_ipclient(TCPSOCKET *sstream)
{
    return sstream->ip_conn;
} 

/* implicitly thread-safe */
char *get_portclient(TCPSOCKET *sstream)
{
    return sstream->port_conn;
} 

/* implicitly thread-safe */
ssize_t tcpwrite(TCPSOCKET *stream, const void *buf, size_t count)
{
    size_t written;
    while (count > 0){
        written = write(stream->fd, buf, count);
        if (written == -1){
            if (errno == EINTR){
                written = 0;
            }else{
                return -1;
            }
        }
        count -= written;
        buf += written;
    }
    return 0;
}

/* implicitly thread-safe */
static int tcp_fillbuf(TCPSOCKET *sstream)
{
    int nread; 
    
    if (sstream->nleft > 0 || sstream->ungotten != EOF){
        return -1;
    }
    do{
        nread = read(sstream->fd, sstream->buf, MAXBUFSIZE);
    }while(nread == -1 && errno == EINTR);
    if(nread == -1){
        return -1;
    }
    sstream->nleft = nread;
    sstream->bufp = sstream->buf;
    return nread;
}

/* implicitly thread-safe */
int tcpgetc(TCPSOCKET *sstream)
{
    int c;

    c = sstream->ungotten;
    if (c != EOF){
        sstream->ungotten = EOF;
        return c;
    }
    if (sstream->nleft == 0 && tcp_fillbuf(sstream) <= 0){
        return EOF;
    }
    --sstream->nleft;
    return *sstream->bufp++;
}

/* implicitly thread-safe */
int tcpungetc(TCPSOCKET *sstream, char c)
{
    if (sstream->ungotten != EOF){
        return 1;
    }
    sstream->ungotten = c;
    return 0;
}

/* implicitly thread-safe */
ssize_t tcpread(TCPSOCKET *stream, int endc, char *buf, size_t count)
{
    int c;
    char *bufstart = buf;

    while (count-- > 0 && (c = tcpgetc(stream)) != EOF && c != endc){
        *buf++ = c;
    }
    if (c == endc && c != EOF){
        *buf++ = c;
    }
    return buf - bufstart;
}
