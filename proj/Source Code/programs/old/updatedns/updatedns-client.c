#include <stdio.h>
#include <stdlib.h>
#define _XOPEN_SOURCE
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <pwd.h>
#include "split.h"
#undef _COMPILING_GETCONFIG_
#include "getconfig.h"

#define USERCONF		".updatedns"
#define ETCCONF			"/etc/updatedns"

static char *Server;
static char *Hostname;
static char *Password;
static char *Port;

struct param_t Parms[] = {
		{"Server", &Server, NULL},
		{"Hostname", &Hostname, NULL},
		{"Password", &Password, NULL},
		{"Port", &Port, "12222"},
		{NULL, NULL, NULL}
};

int connect_to(char *host, int port) {

    int sockfd;
    struct hostent *he;
    struct sockaddr_in server_addr;

    if (!(he = gethostbyname(host))) {
	return -1;
    }
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
	return -2;
    }
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr = *((struct in_addr *)he->h_addr);
    memset(&(server_addr.sin_zero), '\0', 8);
    if (connect(sockfd, (struct sockaddr *)&server_addr,
				sizeof(struct sockaddr)) == -1) {
	return -3;
    }
    return sockfd;
}

int main(int argc, char **argv) {

    int f;
    int i;
    int bs = 256;
    FILE *in_f;
    FILE *out_f;
    char *buf = NULL;
    char *h;
    char *e;
    char salt[] = "$1$........";
    struct passwd *pwd;

    if (!(pwd = getpwuid(getuid()))) {
	printf("Can't read password database.\n");
	exit(1);
    }
    do {
	buf = realloc(buf, bs);
	i = snprintf(buf, bs, "%s/%s", pwd->pw_dir, USERCONF);
	if (i > bs) {
	    bs = i + 1;
	    i += 2;
	}
    } while (i > bs);
    if ((i = getconfig(Parms, buf))) {
	if (i == GETCONFIG_ERR_NO_CONFIG) {
	    i = getconfig(Parms, ETCCONF);
	}
	if (i) {
	    printf("%s\n", getconfig_errstr[i]);
	    exit(1);
	}
    }
    if ((f = connect_to(Server, atoi(Port))) < 0) {
	printf("%i\n", f);
	exit(1);
    }
    in_f = fdopen(f, "r");
    out_f = fdopen(f, "w");
    fgets(buf, bs, in_f);
    if (!(h = strchr(buf, '['))) {
	printf("Malformed header.\n");
	exit(1);
    }
    printf("%s\n", buf);
    printf("%s\n", h);
    h++;
    for (i = 0; i < 8; i++) {
	salt[3 + i] = h[i];
    }
    fprintf(out_f, "%s,%s\n", Hostname, crypt(Password, salt));
    printf("%s,%s\n", Hostname, crypt(Password, salt));
    fflush(out_f);
    fclose(in_f);
    fclose(out_f);
    exit(0);
}
