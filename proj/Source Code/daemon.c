#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>

/* a send() wrapper to push ALL the data across the wire */

int muksend(int sockfd, char *getcall, int *len_getcall) {

  int sent = 0, remaining, cpointer;
  remaining = *len_getcall;

  while (sent < *len_getcall) {
	if ((cpointer = send(sockfd, getcall+sent, remaining, 0)) == -1) {
	  break;
	}
	sent += cpointer;
	remaining -= cpointer;
  }

  if (cpointer == -1) {
	return -1;
  }
  else {
	return sent;
  }

}

/* a send() wrapper to push ALL the data across the wire */

int main(void) {

  char *hostname = "www.freebsd.org";
  char *getcall = "GET /cgi/man.cgi?apropos=2&manpath=FreeBSD+4.3-RELEASE HTTP/1.0\r\n"
	"Host: www.freebsd.org\r\nAccept: */*\r\n\r\n\r\n";
  char *donestuff = "DOWNDONE";
  char buf[1024];
  struct sockaddr_in mysock;
  struct hostent *hst;

  int sockfd, myfd, rbytes, pid, i;
  int headers_parsed = 0, len_getcall = (int)strlen(getcall);

  pid = fork();

  if (pid < 0) {
	exit(1); /* error encountered, no child has been created!
				}

				if (pid != 0) {
				exit(0); /* this is the parent, and hence should be terminated */
  }

  setsid(); /* make the process a group leader, session leader, and lose control tty */

  /* close STDOUT, STDIN, STDERR */

  close(STDIN_FILENO);
  close(STDOUT_FILENO);
  close(STDERR_FILENO);

  /* close STDOUT, STDIN, STDERR */

  signal(SIGHUP, SIG_IGN); /* ignore SIGHUP that will be sent to a child of the process */

  umask(0); /* lose file creation mode mask inherited by parent */
  chdir("/home/pacd00d"); /* change to working dir */

  pid = fork();

  if (pid < 0) {
	exit(1); /* fork() failed, no child process was created! */
  }

  if (pid != 0) {
	exit(0); /* this is the parent, hence should exit */
  }

  /* this is the child process of the child process of the actual calling process */
  /* and can safely be called a grandchild of the original process */ 

  signal(SIGPIPE, SIG_IGN); /* ignore SIGPIPE, for reading, writing to non-opened pipes */
  /* every program using pipes should ignore this signal for 
	 /* being on the safe side */

  if ((hst = gethostbyname(hostname)) == NULL) {
	exit(1); /* you can also use herror() to log the error to a file maybe */
  }

  bzero(&mysock, sizeof(mysock)); /* zero out the struct */
  mysock.sin_family = AF_INET; /* Arpa Internet Protocl */
  memcpy(&mysock.sin_addr, hst->h_addr, sizeof(mysock.sin_addr)); /* ip from hostent */
  mysock.sin_port = htons(80); /* set port */

  if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
	exit(1);
  }

  if (connect(sockfd, (struct sockaddr *)&mysock, sizeof(struct sockaddr_in)) != 0) {
	exit(1);
  }

  if (muksend(sockfd, getcall, &len_getcall) < 0) {
	exit(1);
  }

  /* let's stream the data we recv()ieve */

  myfd = open("/home/pacd00d/freebsd_man.tgz", O_WRONLY|O_CREAT, 0644);
  if (myfd < 0) {
	exit(1); /* file didn't open */
  }

  do {

	rbytes = recv(sockfd, buf, sizeof(buf), 0);

	if (rbytes == 0) {
	  break; /* the server sent a FIN */
	}

	if (rbytes < 0) {
	  if (errno == EINTR) {
		continue; /* if we were interuppted by a signal, then continue */
	  }
	  else {
		exit(1); /* quit otherwise */
	  }

	}

	buf[rbytes] = 0;

	/* parse the http response from the actual content */
	/* the first occurence of \r\n\r\n means that the */
	/* actual content starts from that point */
	/* any data before that needn't be preserved unless */
	/* ofcourse it is wanted, and yes i'm assuming here */
	/* that the syntax is correct, and the file is also */
	/* available to the webserver to present */
	/* you might want to add some error checking for */
	/* complete applications */

	if (!headers_parsed) {
	  for (i = 0; i < rbytes; i++) {
		if ((buf[i] == '\r') && (buf[(i+1)] == '\n') && 
			(buf[(i+2)] == '\r') && (buf[(i+3)] == '\n')) {
		  write(myfd, buf+i+4, (rbytes - (i+4)));
		  headers_parsed = 1;
		  break;
		}
	  }
	  continue;
	}

	write(myfd, buf, rbytes);

  } while (1);

  close(myfd);

  myfd = open("/home/pacd00d/freebsd_man.status", O_WRONLY|O_CREAT, 0644);
  if (myfd < 0) {
	exit(1);
  }

  write(myfd, donestuff, sizeof(donestuff));
  close(myfd);

  exit(0);

}