#include "sockio.h"
#include "sharedbuf.h"
#include "exerror.h"
#include "static_web_example.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define MAXTHREADS 10000
#define SBUFSIZE 1000

struct sharedbuf *sbuf;

void *thread_worker(void *varg);

int main(int argc, char **argv)
{

    if (argc != 3){
        printf("Usage: %s <port> <number-of-threads> \n", argv[0]);
        return 0;
    }
    char *port = argv[1];
    int n_threads = atoi(argv[2]);
    if (n_threads < 0 || n_threads > MAXTHREADS){
        printf("error: too many threads %d (max. limit: %d)\n",
                n_threads, MAXTHREADS);
        return -1;
    }

    /* try first IPv6 and fallback to IPv4 if not supported */
    int fp;
    fp = tcplisten_ipv6(port);
    if (fp != -1){
        printf("IPv6 supported...\n");
    }else{
        printf("IPv6 not supported, falling back to IPv4...\n");
        if ((fp = tcplisten_ipv6(port)) == -1){
            fprintf(stderr, "tcplisten error: cannot create listen socket on port %s\n", port);
            return 1;
        };
    }
    printf("listening on port %s...\n", port);

    /* make shared buffer and create all threads workers */
    sbuf = sharedbuf(SBUFSIZE);
    int i;
    pthread_t tid;
    for (i = 0; i<n_threads; ++i){
        Pthread_create(&tid, NULL, thread_worker, NULL);
    }

    /* add connections to the shared buffer */
    TCPSOCKET *sstream;
    while (1)
    {
        if((sstream = tcpsocket_accept(fp)) == NULL){
            return -1;
        }
        sbuf_add(sbuf, (void *)sstream);
        /* We rely on serve() to close tcpsocket sstream */
    }
}

void *thread_worker(void *varg)
{
    TCPSOCKET *sstream;
    pthread_t tid = pthread_self();
    pthread_detach(tid);
    while(1){
        sstream = sbuf_get(sbuf);
        fprintf(stderr, "thread %lu serving [%s]:%s\n", tid, 
                get_ip_peer(sstream), get_port_peer(sstream));
        if (serve(sstream) != 0){
            fprintf(stderr, "thread %lu force close connection\n", tid);
        }
    }
}