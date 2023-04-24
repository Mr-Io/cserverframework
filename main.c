#include "sockio.h"
#include "static_web_example.h"
#include "exerror.h"
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>

/* child reaper handler */
void sigchild_handler(int signum)
{
    while(waitpid(-1, NULL, WNOHANG) > 0);
}

int process_exec(TCPSOCKET *sstream);

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

    /* signal handler for killing zombies */
    struct sigaction act;
    memset(&act, 0, sizeof act);
    act.sa_handler = sigchild_handler;
    Sigaction(SIGCHLD, &act, NULL);

    /* accept connection loop */
    TCPSOCKET *sstream;
    while (1) {
        if((sstream = tcpsocket_accept(fp)) == NULL){
            return -1;
        }
        int errcode;
        if ((errcode = fork()) == 0){
            return process_exec(sstream);
        }else if (errcode == -1){
            printf("Fork failed?\n");
            return -1;
        }
        tcpsocket_close(sstream);
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

int process_exec(TCPSOCKET *sstream)
{
    int pid;
    pid = getpid();
    fprintf(stderr, "%d process serving client [%s]:%s\n", pid, get_ip_peer(sstream), get_port_peer(sstream));
    if (serve(sstream) != 0){
        fprintf(stderr, "%d process: bad request: force close connection\n", pid);
        return 1;
    }
    return 0;
}