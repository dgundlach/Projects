#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#define COMPILING_SOCKETLIB

#include "socklib.h"

#define POP_EOM			"\n.\r\n"
#define EOM_CHAR		'.'

char socket_error[RESPONSESIZE];

int get_port(char *service, char *protoname)
{
  struct servent *sp;

  if ((sp = getservbyname(service, protoname)) == 0) {
    snprintf(socket_error,RESPONSESIZE,"%s: Unknown service\n",protoname);
    return SOCK_UNKNOWN_SERVICE;
  }
  return htons(sp->s_port);
}

int get_connect(char *serv_name, unsigned short port_num)
{
  struct hostent *hp;
  struct sockaddr_in sa;
  int sockfd;

  if ((hp = gethostbyname(serv_name)) == NULL) {
    snprintf(socket_error,RESPONSESIZE,"%s: Unknown host\n",serv_name);
    return SOCK_NOEXIST;
  }
  bzero((char *) &sa,sizeof(sa));
  bcopy(hp->h_addr, (char *) &(sa.sin_addr), hp->h_length);
  sa.sin_family = AF_INET;
  sa.sin_port = htons(port_num);

  if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    snprintf(socket_error,RESPONSESIZE,
		"Function get_connect: ERROR in setting up socket.\n");
    return SOCK_ERROR;
  }
  if (connect(sockfd,(struct sockaddr *) &sa, sizeof(sa)) < 0) {
    close(sockfd);
    snprintf(socket_error,RESPONSESIZE,
		"Function get_connect: Socket not responding.\n");
    return SOCK_NORESPOND;
  }
  return sockfd;
}

int sockwrite(int sockfd, char *buff, int bufflen)
{
  if ((send(sockfd, buff, bufflen, 0)) < 0) {
    snprintf(socket_error,RESPONSESIZE,
		"Function sockwrite: ERROR in writing to socket.\n");
    return SOCK_IO_ERROR;
  }
  return 0;
}

int sockread(int sockfd, char *buff, int bufflen)
{
  int count, len_of_line = 1;

  if (recv(sockfd, buff, bufflen, MSG_PEEK) < 1) {
    snprintf(socket_error,RESPONSESIZE,
		"Function sockread: ERROR in reading from socket.\n");
    return SOCK_IO_ERROR;
  }
  for (count = 0; (buff[count] != '\n') && (count < (bufflen - 2)); 		count++, len_of_line++);
  recv(sockfd,buff,len_of_line,0);
  buff[len_of_line] = '\0';
  return len_of_line;
}

int sockwriteln(int sockfd, char *buff)
{
  return sockwrite(sockfd, buff, strlen(buff));
}

int sockprintf(int sockfd, char *format, ...)
{
  va_list ap;
  static char buff[8192];
  int bufflen;

  va_start(ap, format);
  bufflen = vsprintf(buff, format, ap);
  va_end(ap);
  return sockwrite(sockfd, buff, bufflen);
}

int sockgetline(int socket, char *buff, int len)
{
  char *ch;

  ch = buff;
  while (--len) {
    if (read(socket, ch, 1) != 1) {
      snprintf(socket_error, RESPONSESIZE,
		"Function sockgetline: Error in reading from socket.\n");
      return SOCK_IO_ERROR;
    }
    if (*ch == '\n') break;
    if (*ch != '\r') buff++;
  }
  *ch = '\0';
  return ch - buff;
}
