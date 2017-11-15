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
#include <sys/stat.h>
#include "daemonize.h"

#include "configure.h"
#include "remount.h"

#define BUF_LEN (10 * (sizeof(struct inotify_event) + NAME_MAX + 1))
 
int main(int argc, char *argv[]) {

	int inotifyFd, wd;
	char buf[BUF_LEN] __attribute__ ((aligned(8)));
	ssize_t numRead;
	char *p;
	char *path = NULL;
	int len = 0;
	int newlen;
	struct inotify_event *event;
	int rc;
	int wc = 0;
	fs_opts *fsOpts = NULL;
	mount_opts *mountOpts = NULL;
	mount_opts *mO = NULL;
	owner_opts *ownerOpts = NULL;
	owner_opts *oO = NULL;


	daemonize(basename(argv[0]), "root", LOG_DAEMON, &daemonize_signal_handler);
	configure(&fsOpts, &ownerOpts, &mountOpts);

	inotifyFd = inotify_init();
	if (inotifyFd == -1) {
		syslog(LOG_ERR, "Error initializing inotify.");
		daemon_clean_up();
	}

	mO = mountOpts;
	while (mO) {
		if (strcmp(mO->peerDir, "*")) {
			wc++;
			wd = inotify_add_watch(inotifyFd, mO->peerDir, IN_DELETE_SELF|IN_DELETE|IN_ISDIR);
			if (wd == -1) {
				syslog(LOG_ERR, "Error adding inotify watch for %s.", mO->peerDir);
				daemon_clean_up();
			}
			mO->wd = wd;
		}
		mO = mO->next;
	}

	oO = ownerOpts;
	while (oO) {
		wc++;
		wd = inotify_add_watch(inotifyFd, oO->dir, IN_CREATE|IN_ISDIR);
		if (wd == -1) {
			syslog(LOG_ERR, "Error adding inotify watch for %s.", oO->dir);
			daemon_clean_up();
		}
		oO->wd = wd;
		oO = oO->next;
	}

	for (;;) {
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

		for (p = buf; p < buf + numRead; ) {
			event = (struct inotify_event *) p;
			if (event->mask & IN_CREATE) {
				oO = ownerOpts;
				while (oO) {
					if (event->wd == oO->wd) {
						newlen = snprintf(path, len, "%s/%s", oO->dir, event->name);
						if (newlen >= len) {
							len = newlen;
							path = realloc(path, len);
							newlen = snprintf(path, len, "%s/%s", oO->dir, event->name);
						}
						chown(path, oO->uid, oO->gid);
						chmod(path, oO->mode);
						break;
					}
					oO = oO->next;
				}
			} else {
				mO = mountOpts;
				while (mO) {
					if (event->wd == mO->wd) {
						if (event->mask & IN_DELETE_SELF) {
							syslog(LOG_INFO, "%s removed by automount.", mO->peerDir);
							wc--;
							if (!wc) {
								syslog(LOG_INFO, "Last watch removed.  Exiting.");
								daemon_clean_up();
							}
						} else {
							rc = remountToggle(event->name, mO->dir);
						}
						break;
					}
					mO = mO->next;
				}
			}
			p += sizeof(struct inotify_event) + event->len;
		}
	}
	daemon_clean_up();
}
