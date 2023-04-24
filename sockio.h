/*
* Some features of the module functions:
* - Thread-safe.
* - Buffered input [#]_.
* - Full duplex, input and output may be interleaved without problem.
* - No short-counts on tpcread/tcpwrite, blocking instead [#]_.
* - No error on signal interrupts.
* - Ipv4/Ipv6 agnostic (except when choosing the type of listening socket).
*/
#include <unistd.h>

typedef struct TCPSOCKET TCPSOCKET;

/* return a TCPSOCKET socket stream on success and -1 on error */
TCPSOCKET *tcpsocket_connect(const char *hostname,const char *port);

/* 
* it return a listening socket file descriptor on success
* and -1 on error
*/
int tcplisten_ipv4(const char *port);
int tcplisten_ipv6(const char *port);

/* 
* It return a TCPSOCKET socket stream on success and NULL on error 
*/
TCPSOCKET *tcpsocket_accept(int listen_socket);

/* 
* It return the ip address of the accepted client connection 
*/
char *get_ip_peer(TCPSOCKET *sstream);
/* 
* It return the port number of the accepted client connection 
*/
char *get_port_peer(TCPSOCKET *sstream);

/* close a connected or accepted TCPSOCKET socket stream */
int tcpsocket_close(TCPSOCKET *tcpf);

/* 
* On success, tcpwrite always write 'count' bytes and return 0.
* tcpwrite return -1 on error.
*/
ssize_t tcpwrite(TCPSOCKET *sstream, const void *buf, size_t count);
/* 
* On success, tcpread return the number of characters readed.
* If tcpread() return a number less than 'count' 
* (including 0), it always indicates an end of file.
* tcpread return -1 on error.
*/
ssize_t tcpread(TCPSOCKET *sstream, int endc, char *buf, size_t count);

/* 
* get and unget a character from a TCPSOCKET socket stream.
* return EOF on error. 
*/
int tcpgetc(TCPSOCKET *sstream);
int tcpungetc(TCPSOCKET *sstream, char c);