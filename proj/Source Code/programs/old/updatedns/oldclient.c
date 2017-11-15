#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include "split.h"

#define USERCONF		".updatedns"
#define ETCCONF			"/etc/updatedns"

static char *Server;
static char *Hostname;
static char *Password;
static char *Port;

static struct splitstr_t *ss = NULL;

struct param_t {
    char *n;
    char **d;
    char *i;
};

struct param_t Parms[] = {
		{"Server", &Server, NULL},
		{"Hostname", &Hostname, NULL},
		{"Password", &Password, NULL},
		{"Port", &Port, "12222"},
		{NULL, NULL}
};

int GetSettings(void) {

    FILE *f;
    char buf[256];
    char *param;
    char *value;
    int i;
    int found;

    for (i = 0; Parms[i].n; i++) {
	if (Parms[i].i) {
	    *(Parms[i].d) = Parms[i].i;
	}
    }
    if (!(f = fopen(USERCONF, "r"))) {
	if (!(f = fopen(ETCCONF, "r"))) {
	    return 1;
	}
    }
    ss = split_ready(ss, 128);
    fgets(buf, sizeof(buf), f);
    while (!feof(f)) {
	if (*buf != '#' && *buf != ';') {
	    ss = split(ss, buf, " \t");
	    if (ss->c) {
		if (ss->c != 2) {
		    fclose(f);
		    return 1;
		}
		found = 0;
		printf("%s = %s\n", ss->s[0], ss->s[1]);
		for (i = 0; Parms[i].n; i++) {
		    if (!strcasecmp(ss->s[0], Parms[i].n)) {
			*(Parms[i].d) = strdup(ss->s[1]);
			found = 1;
			break;
		    }
		}
		if (!found) {
		    fclose(f);
		    return 1;
		}
	    }
	}
	fgets(buf, sizeof(buf), f);
    }
    fclose(f);
    return 0;
}

int main(int argc, char **argv) {

    int i;

    GetSettings();
    for (i = 0; Parms[i].n; i++) {
	printf("%s = %s\n", Parms[i].n, *(Parms[i].d));
    }
    printf("\n");
    printf("Server = %s\n", Server);
    printf("Hostname = %s\n", Hostname);
    printf("Password = %s\n", Password);
    printf("Port = %s\n", Port);



}
