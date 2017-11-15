#define _XOPEN_SOURCE
#define _GNU_SOURCE
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include "split.h"
#include "getpeerip.h"
#include "gohome.h"
#include "salt.h"

#define HEADER		"DNS Update v1.0.0 [%s]\n"
#define PASSWORD_FILE	"passwd"
#define NSUPDATE	"/usr/bin/nsupdate"

static struct splitstr_t *ss = NULL;

int main (int argc, char **argv) {

    char *ip;
    FILE *pf;
    FILE *tf;
    int f;
    char buf[4096];
    char *slt;
    char *host;
    char *password;
    char *crypted;
    char *stored = NULL;
    char *keyfile = NULL;
    char tempfn[] = "updatedns.XXXXXX";
    struct splitstr_t *is = NULL;
    char *ttl;

    if (gohome()) {
	exit(-1);
    }
    if (ip = getpeerip(0)) {
	slt = salt(SALT_MD5);
	fprintf(stdout, HEADER, slt + 3);
	fflush(stdout);
    }
    fgets(buf, sizeof(buf), stdin);
    is = psplit(is, buf, ",: \t");
    if (is->c < 2 || is->c > 3) {
	exit(-1);
    }
    host = is->s[0];
    password = is->s[1];
    if (is->c == 3) {
	ip = is->s[2];
    }
    pf = fopen(PASSWORD_FILE, "r");
    fgets(buf, sizeof(buf), pf);
    while (!feof(pf)) {
	ss = psplit(ss, buf, ":");
        if (!strcasecmp(ss->s[0], host)) {
	    stored = ss->s[1];
	    keyfile = ss->s[2];
	    ttl = ss->s[3];
	    break;
	}
	fgets(buf, sizeof(buf), pf);
    }
    fclose(pf);
    if (!stored) {
	exit(-1);
    }
    crypted = crypt(stored, slt);
    if (strcmp(crypted, password)) {
	exit(-1);
    }
    if ((f = mkstemp(tempfn)) == -1) {
	exit(-1);
    }
    tf = fdopen(f, "w");
    fprintf(tf, "update delete %s\nsend\nupdate add %s %i A %s\nsend\n",
		host, host, ttl, ip);
    fclose(tf);
    snprintf(buf, sizeof(buf), "%s -k %s %s", NSUPDATE, keyfile, tempfn);
    system(buf);
    unlink(tempfn);
    exit(0);
}
