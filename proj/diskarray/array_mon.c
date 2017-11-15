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
#include <sys/mount.h>
#include <uuid/uuid.h>

#include "xalloc.h"
#include "scsi.h"
#include "daemonize.h"
#include "readtextline.h"
#include "array_mon.h"
#include "paths.h"
#include "positiondata.h"

char *position_names[] = {
		"4-1", "3-1", "2-1", "1-1", "4-2", "3-2", "2-2", "1-2",
		"4-3", "3-3", "2-3", "1-3", "4-4", "3-4", "2-4", "1-4",
		"4-5", "3-5", "2-5", "1-5", NULL
};

char *service_names[] = {"autofs", "atalk", "smb", NULL};
char *default_dirs[] = {"Files", "Movies", "Television", NULL};

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
char *tempdir;
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
	int i, j;

	program = basename(argv[0]);
	if (v = getenv("ARRAY_OWNER")) owner = v;
	if (v = getenv("ARRAY_MAKEFS")) {
		makefs = xmalloc(strlen(v) + 50);
		sprintf(makefs, "%s \%s 2>&1 >/dev/null", v);
	}
	if (v = getenv("ARRAY_TUNEFS")) {
		tunefs = xmalloc(strlen(v) + 50);
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
	j = min_unique_id - 1;
	for (i=0; position_names[i]; i++) {
		position_data[j++] = position_names[i];
	}
	tempdir = xmalloc(PATH_MAX);
	strcpy(tempdir, TEMPDIR_PATH);
}

static int cmp_partitions(const void *p1, const void *p2) {
	scsi_partition * const *sp1 = p1;
	scsi_partition * const *sp2 = p2;

	return strncmp((*sp1)->data, (*sp2)->data, NAME_MAX);
}

void setup_device(struct inotify_event *event) {

	int device;
	char *device_name;
	int host_id;
	int unique_id;
	pid_t pid;
	char path[PATH_MAX];
	char command[PATH_MAX];
	char *buf;
	FILE *F;
	FILE *APPLEVOLUMES;
	FILE *AUTODRV;
	FILE *SMBCONF;
	int i;
	uuid_t uuid_unf;
	char *uuid = path;
	
	char pf[4];
	partition_table *parttable;
	scsi_partition **partlist;
	scsi_partition *part;
	struct stat st;

//
// See if the new device is an unpartitioned scsi disk, and that the disk
// falls into our sphere of influence.  Fork a new process to handle it.
//

	device_name = event->name;
	snprintf(path, PATH_MAX, "%s%s", DEV_PATH, device_name);
	if ((!stat(path, &st))
			&& ((device = lookup_scsi_device_number(device_name)) == st.st_rdev) 
			&& !(device & PARTITION_MASK)
			&& !(get_scsi_partitions_name(device_name) > 0)) {
		host_id = get_scsi_host_id(device_name);
		unique_id = get_scsi_unique_id(host_id);
		if ((unique_id >= min_unique_id) && (unique_id <= max_unique_id)) {
			pid = fork();
			switch(pid) {
				case -1:
					syslog(LOG_ERR, "Cannot fork child process.");
					daemon_clean_up();
				case 0:        // Child process continues
					break;
				default:       // Parent process returns
					return;
			}
		} else {
			return;
		}
	} else {
		return;
	}

//
// Child process.  Create and initialize the partition on the disk.  This section of
// the code has teeth, and could potentially ruin your day.
//

	snprintf(command, PATH_MAX, fdisk, unique_id, device_name);
	F = popen(command, "w");
	fprintf(F, fdisk_inst);
	pclose(F);
	snprintf(command, PATH_MAX, makefs, position_data[unique_id - 1], device_name, 1);
	system(command);
	snprintf(command, PATH_MAX, tunefs, device_name, 1);
	system(command);

//
// Create a table of all the partitions in the system and build the new configurations.
//

	parttable = get_scsi_partition_table((void **)position_data);
	partlist = parttable->partitions;
	qsort(partlist, parttable->count, sizeof(scsi_partition *), &cmp_partitions);

//
// Back up the original configuration files.
//

	snprintf(path, PATH_MAX, "%s~", autodrv);
	rename(autodrv, path);
	snprintf(path, PATH_MAX, "%s~", smbconf);
	rename(smbconf, path);
	snprintf(path, PATH_MAX, "%s~", applevolumes);
	rename(applevolumes, path);

//
// Create the new configuration files.  We need part of the old appletalk
// configuration file, so copy it to the new one.
//

	APPLEVOLUMES = fopen(applevolumes, "w");
	AUTODRV = fopen(autodrv, "w");
	SMBCONF = fopen(smbconf, "w");

	while (read_text_line(&buf, "/etc/netatalk/AppleVolumes.default~") == READTEXTLINE_SUCCESS) {
		if (buf && !(strstr(buf, autodrv_dir) == buf)) {
			fprintf(APPLEVOLUMES, buf);
		}
	}

    i = 0;
	while (i < parttable->count) {
		part = partlist[i++];
		if ((part->unique_id >= min_unique_id) && (part->unique_id <= max_unique_id)) {
			if (part->partition_count > 1) {
				snprintf(pf, 4, "-%d", part->device & PARTITION_MASK);
			} else {
				pf[0] = '\0';
			}
			fprintf(APPLEVOLUMES, APPLEVOLUMES_FORMAT, autodrv_dir, part->data, pf, part->data, pf);
			fprintf(SMBCONF, SMBCONF_FORMAT, part->data, pf, part->data, pf, autodrv_dir, part->data, pf, owner);
			if (*part->label) {
				fprintf(AUTODRV, AUTODRV_FORMAT, part->data, pf, fsoptions, "LABEL", part->label);
			} else {
				fprintf(AUTODRV, AUTODRV_FORMAT, part->data, pf, fsoptions, "UUID", part->uuid);
			}
		}
	}

	fclose(APPLEVOLUMES);
	fclose(AUTODRV);
	fclose(SMBCONF);

//
// Reload all the services.
//

	for (i=0; service_names[i]; i++) {
		snprintf(command, PATH_MAX, "/sbin/service %s reload 2>&1 >/dev/null", service_names[i]);
		system(command);
	}

//
// Create our base set of directories in the new volume.  Since we're using automount,
// let the automounter mount the new volume for us. :)
//

	umask(0022);
	for (i=0; default_dirs[i]; i++) {
		snprintf(path, PATH_MAX, "%s%s/%s", autodrv_dir, position_names[unique_id - min_unique_id],
				default_dirs[i]);
		mkdir(path, 0755);
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

//
// I really don't want to uncomment the next line.  What happens if this
// program becomes self-aware?  The lines afterwards should probably be
// commented out or removed if it is done.
//

//	daemonize(program, NULL, LOG_LOCAL6, &daemonize_signal_handler);

	signal(SIGCHLD, &daemonize_signal_handler);
	signal(SIGTSTP, SIG_IGN);
	signal(SIGTTOU, SIG_IGN);
	signal(SIGTTIN, SIG_IGN);
	signal(SIGHUP, &daemonize_signal_handler);
	signal(SIGTERM, &daemonize_signal_handler);

//
// Initialize inotify and set up the directory to watch.
//

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

//
// Main loop.  Read and process the inotify buffer until this process is
// killed.  We're only looking for new SCSI disks here.  When one is inserted,
// fork a new child process to handle it.
//

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
