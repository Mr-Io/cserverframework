#include "static_web_example.h"
#include "sockio.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>

#define MAXLINE 8192
#define MAXWORD 128

int parse_head(TCPSOCKET *sstream, char *method, char *path, char *query, char *version);
int get_header(TCPSOCKET *sstream, char *name, char *value);
void get_filetype(char *filename, char *filetype);
void http_error(TCPSOCKET *sstream, char *cause, char *errnum, 
		        char *shortmsg, char *longmsg);

int serve(TCPSOCKET * sstream)
{
    int errcode;

    /* get method uri version */
    char method[MAXWORD], path[MAXLINE], query[MAXLINE], version[MAXWORD];
    if (parse_head(sstream, method, path, query, version) == -1){
        http_error(sstream, "no data on request", "400", "Bad Request", "malformed request syntax");        
        return -1;
    }
    /* if method is not get, raise non supported error */
    if (strcasecmp(method, "GET") != 0){
        http_error(sstream, "web example only implement GET method", "405", "Method Not Allowed", "Method Not Allowed");
        return -1;
    }

    /* get headers and just ignore them */
    char hname[MAXLINE], hvalue[MAXLINE];
    while((errcode = get_header(sstream, hname, hvalue)) == 1);
    if (errcode != 0){
        http_error(sstream, "error parsing request header", "400", "Bad Request", "malformed request syntax");        
        return -1;
    }

    /* read file and filesize */
    char filename[MAXLINE];
    unsigned filesize = 0;
    strcpy(filename, "./static");
    strcat(filename, path);
    if (path[strlen(path)-1] == '/'){
        strcat(filename, "index.html");
    }
    struct stat statbuf;
    if(stat(filename, &statbuf) == -1){
        http_error(sstream, "file not found", "404", "Not Found", "error on stat()");        
        return -1;
    }
    filesize = statbuf.st_size;

    /* sent response to client */
    char tmp[MAXWORD];
    char response[MAXLINE];
    strcpy(response, "HTTP/1.0 200 OK\r\n");
    strcat(response, "Server: web server framework 0.0\r\n");
    strcat(response, "Connection: close\r\n");
    sprintf(tmp, "Content-length: %d\r\n", filesize);
    strcat(response, tmp);
    get_filetype(filename, tmp);
    strcat(response, "Content-type: ");
    strcat(response, tmp);
    strcat(response, "\r\n");
    strcat(response, "\r\n");
    
    tcpwrite(sstream, response, strlen(response));
    int fd;
    if ((fd = open(filename, O_RDONLY)) == -1){
        http_error(sstream, "file not found", "404", "Not Found", "error on open()");        
        return -1;
    }
    char *pfile = (char *)mmap(NULL, filesize, PROT_READ, MAP_PRIVATE, fd, 0);
    close(fd);
    tcpwrite(sstream, pfile, filesize);
    tcpsocket_close(sstream);
    return 0;
}

/*
 * get_filetype - derive file type from file name
 */
void get_filetype(char *filename, char *filetype) 
{
    if (strstr(filename, ".html"))
	strcpy(filetype, "text/html");
    else if (strstr(filename, ".css"))
	strcpy(filetype, "text/css");
    else if (strstr(filename, ".gif"))
	strcpy(filetype, "image/gif");
    else if (strstr(filename, ".png"))
	strcpy(filetype, "image/png");
    else if (strstr(filename, ".jpg"))
	strcpy(filetype, "image/jpeg");
    else
	strcpy(filetype, "text/plain");
}  

/*
 * clienterror - returns an error message to the client
 */
void http_error(TCPSOCKET *sstream, char *cause, char *errnum, 
		 char *shortmsg, char *longmsg) 
{
    char tmp[MAXWORD];
    char response[MAXLINE];
    sprintf(response, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
    strcat(response, "Content-type: text/html\r\n");
    strcat(response, "\r\n");
    strcat(response, "<html><title>SError</title>");
    strcat(response, "<body bgcolor=""ffffff"">\r\n");
    char buf[MAXLINE];
    sprintf(buf, "%s: %s\r\n", errnum, shortmsg);
    strcat(response, buf);
    sprintf(buf, "<p>%s: %s\r\n", longmsg, cause);
    strcat(response, buf);
    sprintf(buf, "<hr><em>The Server Framework 0.0</em>\r\n");
    strcat(response, buf);

    tcpwrite(sstream, response, strlen(response));
    tcpsocket_close(sstream);
}

int parse_head(TCPSOCKET *sstream, char *method, char *path, char *query, char *version)
{
    char buf[MAXLINE];
    char uri[MAXLINE];
    if(tcpread(sstream, '\n', buf, MAXLINE) <= 0){
        return -1;
    }
    char format[MAXLINE];
    if(sscanf(buf, "%s %s %s", method, uri, version) != 3){ /* no buffer overflow */
        return -1;
    }

    /* get path from uri */
    int pathlen, urilen;
    pathlen = strcspn(uri, "?");
    urilen = strlen(uri);
    strncpy(path, uri, urilen);
    path[pathlen] = '\0';

    /* get query from uri */
    if (pathlen < urilen){
        pathlen++;
        strncpy(query, &uri[pathlen], urilen - pathlen);
        query[urilen - pathlen] = '\0';
    }else{
        query[0] = '\0';
    }

    return 0;
}


int get_header(TCPSOCKET *sstream, char *name, char *value)
{
    char buf[MAXLINE];
    int len;
    if ((len = tcpread(sstream, '\n', buf, MAXLINE)) <= 0){
        return -1;
    }
    if (strncmp(buf, "\r\n", len) == 0){
        return 0;
    } 

    /* get header name */
    int namelen;
    namelen = strcspn(buf, ":");
    if (namelen >= len || len > MAXLINE){
        return -1;
    }
    strncpy(name, buf, namelen);
    name[namelen] = '\0';

    /* get header value */
    int i = namelen;
    while(isblank(buf[++i]));
    strncpy(value, &buf[i], len - i - 2);
    value[len - i - 2] = '\0';

    return 1;
}