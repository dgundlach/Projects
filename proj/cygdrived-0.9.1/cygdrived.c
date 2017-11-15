#include <stdlib.h>
#include <stdio.h>
#include <sys/inotify.h>
#include <limits.h>
#include <mntent.h>
#include <sys/mount.h>
#include <syslog.h>
#include <libgen.h>
#include <string.h>
#include "daemonize.h"

#define MTAB		"/etc/mtab"
#define NEW_MTAB	"/etc/mtab~"
#define NAME_MAX_PLUS	NAME_MAX + 32

int remountMultimedia (char *name) {

	char mountName[NAME_MAX_PLUS];
	FILE *oldMtab, *newMtab;
	struct mntent *ent;
	int rc = 0;
	int changed = 0;
	int mntOpts = MS_MGC_VAL|MS_RDONLY|MS_REMOUNT;
	char *optsStr;

	snprintf(mountName, NAME_MAX_PLUS, "/multimedia/%s", name);
	oldMtab = setmntent(MTAB, "r");
	newMtab = setmntent(NEW_MTAB, "w");
	while (ent = getmntent(oldMtab)) {
		if (!strcmp(ent->mnt_dir, mountName) && hasmntopt(ent, MNTOPT_RW)) {
			if (hasmntopt(ent,"noatime")) {
				mntOpts |= MS_NOATIME;
				optsStr = "ro,noatime";
			} else {
				optsStr = "ro";
			}
			rc = mount(ent->mnt_fsname, ent->mnt_dir, NULL, mntOpts, NULL);
			ent->mnt_opts = optsStr;
			addmntent(newMtab, ent);
			changed = 1;
		} else {
			addmntent(newMtab, ent);
		}
	}
	endmntent(oldMtab);
	endmntent(newMtab);
	if (changed) {
		rename(NEW_MTAB, MTAB);
	} else {
		unlink(NEW_MTAB);
	}
	return rc;
}

#define BUF_LEN (10 * (sizeof(struct inotify_event) + NAME_MAX + 1))
 
int main(int argc, char *argv[]) {

	int inotifyFd, wd, j;
	char buf[BUF_LEN] __attribute__ ((aligned(8)));
	ssize_t numRead;
	char *p;
	struct inotify_event *event;
	int rc;

	daemonize(basename(argv[0]), "root", LOG_DAEMON, &daemonize_signal_handler);

	inotifyFd = inotify_init();
	if (inotifyFd == -1) {
		syslog(LOG_ERR, "Error initializing inotify.");
		daemon_clean_up();
	}

	wd = inotify_add_watch(inotifyFd, "/cygdrive", IN_DELETE_SELF|IN_DELETE|IN_ISDIR);
	if (wd == -1) {
		syslog(LOG_ERR, "Error adding inotify watch for /cygdrive.");
		daemon_clean_up();
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
			if (event->mask & IN_DELETE_SELF) {
				syslog(LOG_INFO, "/cygdrive removed by automount.  Exiting.");
				daemon_clean_up();
			}
			rc = remountMultimedia(event->name);
			p += sizeof(struct inotify_event) + event->len;
		}
	}
	daemon_clean_up();
}
