#include "sockio.h"
#include "static_web_example.h"
#include <stdio.h>

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
        if ((fp = tcplisten_ipv6(port)) == -1){
            fprintf(stderr, "tcplisten error: cannot create listen socket on port %s\n", port);
            return 1;
        };
    }
    printf("listening on port %s...\n", port);

    TCPSOCKET *sstream;
    while (1)
    {
        if((sstream = tcpsocket_accept(fp)) == NULL){
            return -1;
        }
        fprintf(stderr, "[%s]:%s\n", get_ip_peer(sstream), get_port_peer(sstream));
        if (serve(sstream) != 0){
            fprintf(stderr, "bad request: force close connection\n");
        }
        /* We rely on serve() to close tcpsocket sstream */
    }
}