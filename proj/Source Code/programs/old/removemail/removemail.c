#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "socklib.h"

char *host = NULL;
char *user = NULL;
char *passwd = NULL;
unsigned int delmails = 0;
int nmails = 0;

#define BUFFSIZE 	128

char buffer[BUFFSIZE];

#define POP3_GENERR	-10
#define POP3_NOUSER	-11
#define POP3_NOPASS	-12
#define POP3_DELERR	-13
#define POP3_QUITERR	-14
#define OK_TOK		"+OK"
#define ERR_TOK		"-ERR"
#define DEFAULT_HOST	"mail.spiff.net"

void process_args (int argc, char **argv)
{
  if (argc > 1) host = argv[1];
  if (argc > 2) user = argv[2];
  if (argc > 3) passwd = argv[3];
  if (argc > 4) delmails = strtoul(argv[4],NULL,10);

  if (!host) {
    printf("\nThis program removes a specified number of mails for a user.\n");
    printf("\nHost [%s] : ", DEFAULT_HOST);
    host = malloc(256);
    fgets(host,256,stdin);
    *(host + strlen(host) - 1) = '\0';
    if (!*host) {
      free(host);
      host = DEFAULT_HOST;
    }
  }
  if (!*host) exit(0);
  if (!user) {
    printf("Username : ");
    user = malloc(256);
    fgets(user,256,stdin);
    *(user + strlen(user) - 1) = '\0';
  }
  if (!*user) exit(0);
  if (!passwd) {
    passwd = getpass("Password : ");
  }
  if (!*passwd) exit(0);
}

int pop3_connect (void)
{
  int port = 110, resp, sockfd;

  port = get_port("pop3",SOCK_TCP_PROTO);
  if (port < 0) {
    return port;
  }
  if ((sockfd = get_connect(host, port)) < 1) {
    return sockfd;
  }
  if ((resp = sockread(sockfd, buffer, BUFFSIZE)) < 1) {
    return resp;
  }
  if (strstr(buffer, OK_TOK) != buffer) {
    snprintf(socket_error,RESPONSESIZE,
		"Function pop3_connect: General Error.\n");
    return POP3_GENERR;
  }
  if (resp = sockprintf(sockfd, "user %s\n", user)) {
    return resp;
  }
  if ((resp = sockread(sockfd, buffer, BUFFSIZE)) < 1) {
    return resp;
  }
  if (strstr(buffer, OK_TOK) != buffer) {
    snprintf(socket_error,RESPONSESIZE,
		"Function pop3_connect: User does not exist.\n");
    return POP3_NOUSER;
  }
  if (resp = sockprintf(sockfd, "pass %s\n", passwd)) {
    return resp;
  }
  if ((resp = sockread(sockfd, buffer, BUFFSIZE)) < 1) {
    return resp;
  }
  if (strstr(buffer, OK_TOK) != buffer) {
    snprintf(socket_error,RESPONSESIZE,
		"Function pop3_connect: Bad user/pass combination.\n");
    return POP3_NOPASS;
  }
  return sockfd;
}

int pop3_get_nmails (int sockfd)
{
  int resp;
  int count;

  if (resp = sockprintf(sockfd, "list\n", user)) {
    return resp;
  }
  if ((resp = sockread(sockfd, buffer, BUFFSIZE)) < 1) {
    return resp;
  }
  if (strstr(buffer, OK_TOK) != buffer) {
    snprintf(socket_error,RESPONSESIZE,
		"Function pop3_get_nmails: Error in retrieving list.\n");
    return POP3_GENERR;
  }
  if ((resp = sockread(sockfd, buffer, BUFFSIZE)) < 1) {
    return resp;
  }
  printf("\n");
  for (count = 0; *buffer != '.'; count++) {
    printf ("User has %d mails.\r", count + 1);
    if ((resp = sockread(sockfd, buffer, BUFFSIZE)) < 1) {
      return resp;
    }
  }
  if (count) printf("\n"); else printf("User has no mail.\n");

  return count;
}

int pop3_del_mails(int sockfd)
{
  int count;
  int resp;

  for (count = 1; count <= delmails; count++) {
    printf("Deleting message: %d\r",count);
    if (resp = sockprintf(sockfd, "dele %d\n",count)) {
      return resp;
    }
    if ((resp = sockread(sockfd, buffer, BUFFSIZE)) < 1) {
      return resp;
    }
    if (strstr(buffer, OK_TOK) != buffer) {
      snprintf(socket_error,RESPONSESIZE,
		"Function pop3_del_mails: Error in deleting message.\n");
      return POP3_DELERR;
    }
  }
  printf("\nClosing.\n");
  if (resp = sockprintf(sockfd, "quit\n")) {
    return resp;
  }
  if ((resp = sockread(sockfd, buffer, BUFFSIZE)) < 1) {
    return resp;
  }
  if (strstr(buffer, OK_TOK) != buffer) {
    snprintf(socket_error,RESPONSESIZE,
		"Function pop3_del_mails: Error in quitting.\n");
    return POP3_DELERR;
  }
  return 0;
}

int main (int argc, char **argv)
{
  int sockfd;
  int count;
  int resp;
  char temp[20];

  process_args(argc, argv);
  sockfd = pop3_connect();
  if (sockfd < 0) {
    printf("%s",socket_error);
    exit(sockfd);
  }
  count = pop3_get_nmails(sockfd);
  if (count < 0) {
    printf("%s",socket_error);
    close(sockfd);
    exit(count);
  }
  if (!count) {
    printf("\nClosing.\n");
    exit(0);
  }
  if (!delmails) {
    printf ("\nNumber to remove : ");
    fgets(temp, 20, stdin);
    delmails = strtoul(temp,NULL,10);
    if (delmails) printf("\n");
  }
  if (delmails > count) delmails = count;
  if (delmails) {
    resp = pop3_del_mails(sockfd);
    if (resp < 0) {
      printf("%s",socket_error);
    }
  } else printf("\nClosing.\n");
  close(sockfd);
  exit(resp);
}
