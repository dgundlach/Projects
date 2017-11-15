/*
 * Uncomment STRIP_CR if we're using a UNIX system, or a system that
 * ends lines with a line feed, not a CR-LF pair.
 */
#define STRIP_CR			1

#define RESPONSESIZE			80

#define SOCK_UDP_PROTO			"udp"
#define SOCK_TCP_PROTO			"tcp"

#define SOCK_ERROR			-1
#define SOCK_NOEXIST			-2
#define SOCK_NORESPOND			-3
#define SOCK_INVALID_RESPONSE		-4
#define SOCK_UNKNOWN_SERVICE		-5
#define SOCK_IO_ERROR			-6
#define OUT_OF_MEMORY			-7

#ifndef COMPILING_SOCKETLIB
extern char socket_error[RESPONSESIZE];
#endif

int get_port (char *service, char *protoname);
int get_connect (char *serv_name, unsigned short port_num);
int sockwrite (int sockfd, char *buff, int bufflen);
int sockread (int sockfd, char *buff, int bufflen);
int sockwriteln (int sockfd, char *buff);
int sockprintf (int sockfd, char *format, ...);
int sockgetline (int sockfd, char *buff, int len);
