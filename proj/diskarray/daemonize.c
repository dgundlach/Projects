#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#include <syslog.h>
#include <signal.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <libgen.h>
#include <string.h>
#include <stdlib.h>

#include "daemonize.h"

char *daemon_lock_file;
int daemon_status;

void daemonize_signal_handler(int signum) {

	int w;

	switch(signum) {
		case SIGHUP:
			break;
		case SIGTERM:
			daemon_clean_up();
			break; // This will never get executed
		case SIGCHLD:
			w = wait(&daemon_status);
	}
}

void daemon_clean_up(void) {

	int i;

 	syslog(LOG_INFO, "Exiting.");
	closelog();
 	for (i=getdtablesize(); i>=0; --i) close(i);
	unlink(daemon_lock_file);
	kill(0, SIGTERM);
	exit(0);
}

void daemonize_set_user(char *user) {

	struct passwd *pwent;

	if (!getuid()) {
		if (!(pwent = getpwnam(user))) {
			syslog(LOG_ERR, "No such user: %s", user);
			daemon_clean_up();
		}
		setgid(pwent->pw_gid);
		setuid(pwent->pw_uid);
	}
}

void daemonize(char *program, char *user, int facility, void (*sig_handler)(int)) {

	int i, p, pf;
	char str[10];
	char pfn[256];

	if (getppid() == 1) return;
	p = fork();
	switch(p) {
		case -1:
			exit(1);
		case 0:        // Child process continues
			break;
		default:       // Parent process returns
			exit(0);
	}
	openlog(program, LOG_PID, facility);
	syslog(LOG_INFO, "Starting.");
	setsid();
	for (i=getdtablesize(); i>=0; --i) close(i);
	i=open("/dev/null", O_RDWR);
	dup(i);
	dup(i);
	if (user) daemonize_set_user(user);
	snprintf(pfn, sizeof(pfn), "/var/lock/%s/pid", program);
	if ((pf = open(pfn, O_RDWR | O_CREAT, 0640)) < 0) {
		syslog(LOG_ERR, "Cannot open lock file.");
		daemon_clean_up();
	}
	if (lockf(pf, F_TLOCK, 0) < 0) {
		syslog(LOG_ERR, "Cannot get exclusive lock.");
		daemon_clean_up();
	}
	sprintf(str, "%d\n", getpid());
	write(pf, str, strlen(str));
	signal(SIGCHLD, sig_handler);
	signal(SIGTSTP, SIG_IGN);
	signal(SIGTTOU, SIG_IGN);
	signal(SIGTTIN, SIG_IGN);
	signal(SIGHUP, sig_handler);
	signal(SIGTERM, sig_handler);
}
