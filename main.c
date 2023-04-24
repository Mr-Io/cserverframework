#include "sockio.h"
#include "static_web_example.h"
#include "exerror.h"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/wait.h>

void *thread_exec(void *vargp);

int main(int argc, char **argv)
{

    if (argc != 2){
        printf("Usage: %s <port>\n", argv[0]);
        return 0;
    }
    char *port = argv[1];

    /* try first IPv6 and fallback to IPv4 if not supported */
    int fp;
    fp = tcplisten_ipv6(port);
    if (fp != -1){
        printf("IPv6 supported...\n");
    }else{
        printf("IPv6 not supported, falling back to IPv4...\n");
        if ((fp = tcplisten_ipv4(port)) == -1){
            fprintf(stderr, "tcplisten error: cannot create listen socket on port %s\n", port);
            return 1;
        }
    }
    printf("listening on port %s...\n", port);

    /* accept connection loop */
    TCPSOCKET *sstream;
    while (1) {
        if((sstream = tcpsocket_accept(fp)) == NULL){
            return -1;
        }
        pthread_t tid;
        Pthread_create(&tid, NULL, thread_exec, (void *)sstream);
        /* CLOSING THE TCPSOCKET stream
        * 1. multiprocessing: within the child process is not important
        *    because the process is gonna return and get reaped anyways.
        *    BUT it is very important to close it within the parent process.
        * 2. multithreading: do NOT close it within the main thread! 
        *    the responsibility to close the TCPSOCKET stream rely on
        *    the serving threads that its using it.
        * 3. multiplexing: ??????
        */
    }
}

void *thread_exec(void * vargp)
{
    /* detach the thread*/
    pthread_t tid = pthread_self();
    Pthread_detach(tid);

    /* thread logic */
    TCPSOCKET *sstream = (TCPSOCKET *)vargp;
    fprintf(stderr, "%ld thread serving client [%s]:%s\n", tid, get_ip_peer(sstream), get_port_peer(sstream));
    if (serve(sstream) != 0){
        fprintf(stderr, "%lu thread: bad request: force close connection\n", tid);
    }
    return NULL;
}