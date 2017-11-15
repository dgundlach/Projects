#include <stdio.h>
#include <signal.h>
#include <libgen.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdarg.h>
#include <syslog.h>
#include <sys/inotify.h>
#include <dirent.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <pwd.h>
#include <glib.h>

#include "scsi.h"
#include "daemonize.h"
#include "array_mon.h"
#include "paths.h"

char *position_names[] = {
							"4-1", "3-1" "2-1", "1-1",
							"4-2", "3-2" "2-2", "1-2",
							"4-3", "3-3" "2-3", "1-3",
							"4-4", "3-4" "2-4", "1-4",
							"4-5", "3-5" "2-5", "1-5"
						 };

char *service_names[] = {"autofs", "atalk", "smb", "linkmoviesd", ""};
char *default_dirs[] = {"Files", "Movies", "Television", ""};

char *owner = OWNER;
char *fdisk = FDISK_COMMAND;
char *fdisk_inst = FDISK_INSTRUCTIONS;
char *makefs = MAKEFS_COMMAND;
char *tunefs = TUNEFS_COMMAND;
char *fsoptions = FSOPTIONS;
char *applevolumes = APPLEVOLUMES_PATH;
char *smbconf = SMBCONF_PATH;
char *autodrv = AUTODRV_PATH;
char *dev_dir = DEV_PATH;
char *autodrv_dir = "/drv/";
char *program;
int min_unique_id = MIN_UNIQUE_ID;
int max_unique_id = MAX_UNIQUE_ID;
uid_t ouid = 0;
gid_t ogid = 0;

void usage(void) {

	printf("Usage: %s\n", program);
	exit(1);
}

void initialize(int argc, char **argv) {

	char *v;
	struct passwd *pwent;

	program = basename(argv[0]);
	if (v = getenv("ARRAY_OWNER")) owner = v;
	if (v = getenv("ARRAY_MAKEFS")) {
		makefs = malloc(strlen(v) + 50);
		sprintf(makefs, "%s \%s 2>&1 >/dev/null", v);
	}
	if (v = getenv("ARRAY_TUNEFS")) {
		tunefs = malloc(strlen(v) + 50);
		sprintf(tunefs, "%s \%s 2>&1 >/dev/null", v);
	}
	if (v = getenv("ARRAY_FSOPTIONS")) fsoptions = v;
	if (v = getenv("ARRAY_APPLEVOLUMES")) applevolumes = v;
	if (v = getenv("ARRAY_SMBCONF")) smbconf = v;
	if (v = getenv("ARRAY_AUTODRV")) autodrv = v;
	if (pwent = getpwnam(owner)) {
		ouid = pwent->pw_uid;
		ogid = pwent->pw_gid;
	}
}

void setup_device(struct inotify_event *event) {

	dev_t device;
	char *device_name;
	int host_id;
	int unique_id;
	pid_t pid;
	char path[PATH_MAX];
	char command[PATH_MAX];
	char *buf;
	scsi_host **scsi_host_table = NULL;
	scsi_host *h;
	GHashTable *uuid_table = NULL;
	FILE *F;
	FILE *APPLEVOLUMES;
	FILE *AUTODRV;
	FILE *SMBCONF;
	int i;
	char *colrow;
	char *uuid;

/*
 * See if the new device is an unpartitioned scsi disk, and that the disk
 * falls into our sphere of influence.  Fork a new process to handle it.
 */

	if (device = is_unpartitioned_scsi_disk(event->name)) {
		host_id = get_scsi_host_id(event->name);
		unique_id = get_scsi_unique_id(host_id);
		if ((unique_id >= min_unique_id) && (unique_id <= max_unique_id)) {
			pid = fork();
			switch(pid) {
				case -1:
					syslog(LOG_ERR, "Cannot fork child process.");
					daemon_clean_up();
				case 0:        /* Child process continues */
					break;
				default:       /* Parent process returns */
					return;
			}
		} else {
			return;
		}
	} else {
		return;
	}

/*
 * Child process.  Create and initialize the partition on the disk.
 */

	snprintf(command, PATH_MAX, fdisk, event->name);
	F = popen(command, "w");
	fprintf(F, fdisk_inst);
	pclose(F);
	snprintf(command, PATH_MAX, makefs, event->name, 1);
	system(command);
	snprintf(command, PATH_MAX, tunefs, event->name, 1);
	system(command);

	snprintf(path, PATH_MAX, "%s~", autodrv);
	rename(autodrv, path);
	snprintf(path, PATH_MAX, "%s~", smbconf);
	rename(smbconf, path);
	snprintf(path, PATH_MAX, "%s~", applevolumes);
	rename(applevolumes, path);

	APPLEVOLUMES = fopen(applevolumes, "w");
	AUTODRV = fopen(autodrv, "w");
	SMBCONF = fopen(smbconf, "w");

	while (read_text_line(&buf, path)) {
		if (!(strstr(buf, autodrv_dir) == buf)) {
			fprintf(APPLEVOLUMES, buf);
		}
	}
	scsi_host_table = get_scsi_host_table(scsi_host_table);
	for (i=0; i<256; i++) {
		if ((h = scsi_host_table[i]) && (h->unique_id >= min_unique_id)
									 && (h->unique_id <= max_unique_id)) {
			colrow = position_names[h->unique_id - min_unique_id];
			fprintf(APPLEVOLUMES, APPLEVOLUMES_FORMAT, autodrv_dir, colrow, colrow);
			fprintf(AUTODRV, AUTODRV_FORMAT, autodrv_dir, colrow, fsoptions, h->uuid[1]);
			fprintf(SMBCONF, SMBCONF_FORMAT, colrow, colrow, autodrv_dir, colrow, owner);
		}
	}
	fclose(APPLEVOLUMES);
	fclose(AUTODRV);
	fclose(SMBCONF);
	for (i=0; *service_names[i]; i++) {
		snprintf(command, PATH_MAX, "/sbin/service %s reload 2>&1 >/dev/null", service_names[i]);
	}
	colrow = position_names[unique_id - min_unique_id];
	umask(0777);
	for (i=0; *default_dirs[i]; i++) {
		snprintf(path, PATH_MAX, "%s%s/%s", autodrv_dir, colrow, default_dirs[i]);
		mkdir(path, 0644);
		chown(path, ouid, ogid);
	}
	if (pid) return;
	exit(0);
}

int main(int argc, char **argv) {

	int fd;
	int watch;
	char buf[BUF_LEN];
	char path[PATH_MAX];
	int len, i, c;
	struct inotify_event *event;
	pid_t pid;
	DIR *basedir;
	struct dirent *direntry;

	initialize(argc, argv);
//	daemonize(program, NULL, LOG_LOCAL6, &daemonize_signal_handler);

/*
 * Initialize inotify and set up the directory to watch.
 */

	fd = inotify_init();
	if (fd < 0) {
		syslog(LOG_ERR, "Cannot initialize inotify.");
		daemon_clean_up();
	}
	watch = inotify_add_watch(fd, DEV_PATH, IN_CREATE);
	if (watch < 0) {
		syslog(LOG_ERR, "Cannot add inotify watch: %s.", DEV_PATH);
		daemon_clean_up();
	}

/*
 * Main loop.  Read and process the inotify buffer until this process is
 * killed.  We're only looking for new SCSI disks here.  When one is inserted,
 * fork a new child process to handle it.
 */

	while (1) {
		i = 0;
		len = read(fd, buf, BUF_LEN);
		if (len < 0) {
			if (!(errno == EINTR)) {
				syslog(LOG_INFO, "Inotify read error.");
			}
		}
		while (i < len) {
			event = (struct inotify_event *) &buf[i];
			if ((event->mask & IN_CREATE) && !(event->mask & IN_ISDIR)) {
				setup_device(event);
			}
			i += EVENT_SIZE + event->len;
		}
	}
}
