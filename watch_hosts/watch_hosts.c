#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/inotify.h>
#include <limits.h>
#include <mntent.h>
#include <sys/mount.h>
#include <syslog.h>
#include <libgen.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h>
#include <signal.h>
#include <fcntl.h>
#include "daemonize.h"

#define BUF_LEN (10 * (sizeof(struct inotify_event) + NAME_MAX + 1))
#define USER "dan"
#define HOSTS "hosts"
#define HOSTS_HOME "dnsmasq"
#define DNSMASQ_PID "/var/run/dnsmasq/dnsmasq.pid"

int main(int argc, char *argv[]) {

	int inotifyFd, fd, wd;
	char buf[BUF_LEN] __attribute__ ((aligned(8)));
	char pid_buf[16];
	ssize_t numRead, pidRead;
	char *p;
	struct inotify_event *event;
	pid_t pid;
	struct passwd *pwd;
	char *watchpath;
	fd_set watch_set;

	daemonize(basename(argv[0]), "root", LOG_DAEMON, &daemonize_signal_handler);

	if (!(pwd = getpwnam(USER))) {
		syslog(LOG_ERR, "Cannot fetch password entry for user %s.", USER);
		daemon_clean_up();
	}
	watchpath = malloc(sizeof(HOSTS_HOME) + 2 + strlen(pwd->pw_dir));
	sprintf(watchpath, "%s/%s", pwd->pw_dir, HOSTS_HOME);

	inotifyFd = inotify_init();
//	FD_ZERO(&watch_set);
//	FD_SET(inotifyFd, &watch_set);

	if (inotifyFd == -1) {
		syslog(LOG_ERR, "Error initializing inotify.");
		daemon_clean_up();
	}

	wd = inotify_add_watch(inotifyFd, watchpath, IN_ALL_EVENTS);
	if (wd == -1) {
		syslog(LOG_ERR, "Error adding inotify watch for %s.", watchpath);
		daemon_clean_up();
	}

	for (;;) {
//		select(inotifyFd, &watch_set, NULL, NULL, NULL);
		numRead = read(inotifyFd, buf, BUF_LEN);
		if (numRead == 0) {
			syslog(LOG_ERR, "Read from inotify fd returned 0.");
			daemon_clean_up();
		}

		if (numRead == -1) {
			syslog(LOG_ERR, "Inotify read error.");
			daemon_clean_up();
		}

		/* Process all of the events in buffer returned by read() */

		printf ("Hello\n");
		for (p = buf; p < buf + numRead; ) {
		printf ("Hello2\n");
			event = (struct inotify_event *) p;
			if (event->mask & IN_MOVED_TO && !strcmp(HOSTS, event->name)) {
				syslog(LOG_INFO, "Reloading dnsmasq configuration.");

				/* Make sure the file is created */

				if ((fd = open(DNSMASQ_PID, O_RDONLY)) == -1) {
					syslog(LOG_ERR, "Cannot determine PID of dnsmasq.");
				} else {
					pidRead = read(fd, pid_buf, sizeof(pid_buf) - 1);
					if (pidRead) {
						pid = (pid_t)strtol(pid_buf, NULL, 10);
						kill(pid, SIGHUP);
					}
					close(fd);
				}
			}
			p += sizeof(struct inotify_event) + event->len;
		}
	}
	daemon_clean_up();
}
