#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <pwd.h>
#include <libgen.h>
#include <syslog.h>

int  d_log_facility = LOG_LOCAL6;
int  d_log_events   = 0;
char *d_program     = NULL;
char *d_pid_file    = NULL;
char *d_user        = NULL;

void dInternalCleanUp(void) {

	int i;

	if (d_log_events) {
		syslog(LOG_INFO, "Exiting.");
		closelog();
	}
	for (i=getdtablesize(); i>=0; --i) close(i);
	unlink(d_pid_file);
	kill(0, SIGTERM);
	exit(0);
}

void (*dCleanUp)(void) = &dInternalCleanUp;

void dSetProgramName(char *name, char *pidname) {

	int size;
	int fd;
	struct stat st;

	d_program = basename(name);
	size = 16 + strlen(d_program);
	if (pidname) {
		d_pid_file = malloc(size + strlen(pidname));
		sprintf(d_pid_file, "/var/run/%s/%s.pid", d_program, pidname);
	} else {
		d_pid_file = malloc(size);
		sprintf(d_pid_file, "/var/run/%s.pid", d_program);
	}
	if (!stat(d_pid_file, &st)) {
		if (d_log_events) {
			syslog(LOG_ERR, "PID file exists: %s", d_pid_file);
		}
		dCleanUp;
	}
}

void dSetUser(void) {

	struct passwd *pwent;
	char lf[strlen(d_pid_file) + 1];
	char *lp;

	if (strcmp(d_user, "root") && !getuid()) {
		if (!(pwent = getpwnam(d_user))) {
			if (d_log_events) {
				syslog(LOG_ERR, "No such user: %s", d_user);
			}
			dCleanUp();
		}
		strcpy(lf, d_pid_file);
		lp = dirname(lf);
		if (strcmp(lp, "/var/run")) {
			chown(lp, pwent->pw_uid, pwent->pw_gid);
		}
		chown(d_pid_file, pwent->pw_uid, pwent->pw_gid);
		setgid(pwent->pw_gid);
		setuid(pwent->pw_uid);
	}
}

void dDaemonize(void) {

	int i, pid, pf;
	char str[10];
	char lf[strlen(d_pid_file) + 1];
	char *lp;

	if (getppid() == 1) return;
	pid = fork();
	switch(pid) {
		case -1:
			exit(1);
		case 0:        /* Child process continues */
			break;
		default:       /* Parent process returns */
			exit(0);
	}
	if (d_log_events) {
		openlog(d_program, LOG_PID, d_log_facility);
		syslog(LOG_INFO, "Starting.");
	}
	setsid();
	for (i=getdtablesize(); i>=0; --i) close(i);
	i=open("/dev/null", O_RDWR);
	dup(i);
	dup(i);
	strcpy(lf, d_pid_file);
	lp = dirname(lf);
	if (strcmp(lp, "/var/run")) {
		umask(022);
		mkdir(lp, 0755);
	}
	if ((pf = open(d_pid_file, O_RDWR | O_CREAT, 0640)) < 0) {
		if (d_log_events) {
			syslog(LOG_ERR, "Cannot open PID file.");
		}
		dCleanUp();
	}
	if (lockf(pf, F_TLOCK, 0) < 0) {
		syslog(LOG_ERR, "Cannot get exclusive lock.");
		dCleanUp();
	}
	sprintf(str, "%d\n", getpid());
	write(pf, str, strlen(str));
	if (d_user) {
		dSetUser();
	}
}
