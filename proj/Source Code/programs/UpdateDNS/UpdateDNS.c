#define _XOPEN_SOURCE
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <unistd.h>
#include <gdbm.h>
#include <time.h>
#include <getopt.h>
#include "GetPeerIP.h"
#include "MD5Digest.h"
#include "Command.h"
#include "SplitArgs.h"

#define HEADER          "DNS Update v1.0.0 <%i.%i@%s>\n"
#ifndef CONF_DIR
#define CONF_DIR        "/etc/updatedns"
#endif
#ifndef KEYFILE
#define KEYFILE         "updatedns.key"
#endif
#define PASSWORD_FILE   "passwd"
#define PASSWORD_DB     "passwd.db"
#define NSUPDATE        "/usr/bin/nsupdate"

int auth_cmd(int, char **);
int update_cmd(int, char **);
int quit_cmd(int, char **);
int nosuch_cmd(int, char **);

struct cstring {
    int c;
    char *s;
};

struct commands cmds[] = {
                {"auth", &auth_cmd, NULL},
                {"update", &update_cmd, NULL},
                {"quit", &quit_cmd, NULL},
                {"exit", &quit_cmd, NULL},
                {NULL, &nosuch_cmd, NULL}
};

int             authorized      = 0;
int             quit            = 0;
int             do_update       = 0;
int             ttl             = 600;
struct cstring  *vh;
char            *ip;
char            hostname[64];
char            *conf_dir       = CONF_DIR;
char            *keyfile        = KEYFILE;
char            tempfn[]        = "updatedns.XXXXXX";
time_t          now;
pid_t           pid;
FILE            *tf             = NULL;

/*
    fclose(tf);
    snprintf(buf, sizeof(buf), "%s -k %s %s", NSUPDATE, keyfile, tempfn);
    system(buf);
    unlink(tempfn);
*/

int auth_cmd(int argc, char **argv) {

    GDBM_FILE dbf;
    datum user;
    datum record;
    int argcount;
    int p;
    int i;
    char *digest;
    char *buf;
    char **pwd;

    if (argc != 3) {
        fprintf(stdout, "-ERR Usage: AUTH <user> <digest>\n");
        fflush(stdout);
        quit = 1;
        return 1;
    }
    if (authorized) {
        fprintf(stdout, "-ERR Already authorized.\n");
        fflush(stdout);
        return 1;
    }
    user.dptr = argv[1];
    user.dsize = strlen(argv[1]);
    if (!(dbf = gdbm_open(PASSWORD_DB, 4096, GDBM_READER, 0, 0))) {
        fprintf(stdout, "-ERR Cannot open AUTH database.\n");
        fflush(stdout);
        quit = 1;
        return 1;
    }
    record = gdbm_fetch(dbf, user);
    gdbm_close(dbf);
    if (record.dptr) {
        pwd = SplitArgs(record.dptr, ':', &argcount);
        buf = malloc(256);
        p = snprintf(buf, 256, "<%i.%i@%s>%s", pid, (int)now, hostname, pwd[1]);
        if (p >= 256) {
            buf = realloc(buf, p + 2);
            p = snprintf(buf, p + 2, "<%i.%i@%s>%s", pid, (int)now, hostname, pwd[1]);
        }
        digest = MD5Digest(buf);
        free(buf);
        if (!strcmp(digest, argv[2])) {
            authorized = 1;
            vh = malloc((argcount - 1) * sizeof(struct cstring));
            for (i = 0; i < argcount - 2; i++) {
                vh[i].s = pwd[2 + i];
                vh[i].c = strlen(pwd[2 + i]);
            }
            vh[i].s = NULL;
            vh[i].c = 0;
        }
    }
    if (!authorized) {
        fprintf(stdout, "-ERR Authorization failed.\n");
        fflush(stdout);
        quit = 1;
        return 1;
    }
    return 0;
}

int update_cmd(int argc, char **argv) {

    struct cstring host;
    int i;
    int offset;

    if (!authorized) {
        fprintf(stdout, "-ERR AUTH first.\n");
        fflush(stdout);
        return 1;
    }
    host.s = argv[1];
    host.c = strlen(argv[1]);
    for (i = 0; vh[i].s; i++) {
        if (*(vh[i].s) == '.') {
            offset = host.c - vh[i].c;
            if (offset > 0 && !strcasecmp(host.s + offset, vh[i].s)) {
                break;
            }
        } else {
            if (host.c == vh[i].c && !strcasecmp(host.s, vh[i].s)) {
                break;
            }
        }
    }
    if (vh[i].s) {
        fprintf(tf, "update delete %s\nsend\nupdate add %s %i A %s\nsend\n",
                                hostname, hostname, ttl, ip);
        do_update = 1;
        fprintf(stdout, "+OK Host added.\n");
        fflush(stdout);
        return 0;
    }
    return 1;
}

int quit_cmd(int argc, char **argv) {

    char buf[256];

    fclose(tf);
    if (do_update) {
        snprintf(buf, sizeof(buf), "%s -k %s %s", NSUPDATE, keyfile, tempfn);
        system(buf);
    }
    unlink(tempfn);
    quit = 1;    
    fprintf(stdout, "+OK\n");
    fflush(stdout);
    return 0;
}

int nosuch_cmd(int argc, char **argv) {

    fprintf(stdout, "-ERR No such command.\n");
    fflush(stdout);
    return 1;
}

void initialize(int argc, char **argv) {

    int f;
    char *p;

    while (1) {
        int c = getopt(argc, argv, "d:k:");
        if (c == -1) {
            break;
        }
        switch (c) {
        case 'd':
            conf_dir = argv[optind - 1];
            break;
        case 'k':
            keyfile = argv[optind - 1];
            break;
        default:
            if ((p = strrchr(argv[0], '/'))) {
                p++;
            } else {
                p = argv[0];
            }
            printf("Usage: %s [-d <conf dir>]\n", p);
            exit(1);
        }
    }
    if (!(ip = GetPeerIP(0))) {
        exit(1);
    }
    if (!chdir(conf_dir)) {
        exit(1);
    }
    pid = getpid();
    now = time(NULL);
    gethostname(hostname, sizeof(hostname));
    fprintf(stdout, HEADER, pid, (int)now, hostname);
    fflush(stdout);
    if ((f = mkstemp(tempfn)) == -1) {
        exit(-1);
    }
    tf = fdopen(f, "w");
}

int main (int argc, char **argv) {

    char buf[4096];
    int  code;

    initialize(argc, argv);
    fgets(buf, sizeof(buf), stdin);
    while (!quit && !feof(stdin)) {
        code = Command(buf, cmds);
        fgets(buf, sizeof(buf), stdin);
    }
    exit(0);



}
