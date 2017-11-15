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

#include "scsi.h"
#include "daemonize.h"

#define EVENT_SIZE		(sizeof(struct inotify_event))
#define BUF_LEN			(1024 * (EVENT_SIZE + 16))
#define PATH_LEN			1024
#define COMMAND_LEN		1024

#define DEV_DIR			"/dev/"
#define UDEV_JITTER		5

#define OWNER "dan"
#define MAKEFS "/sbin/mke2fs -j -m0 %s 2>&1 >/dev/null"
#define TUNEFS "/sbin/tune2fs -c0 %s 2>&1 >/dev/null"
#define FSOPTIONS "-fstype=ext3,noatime"

#define APPLEVOLUMES "/etc/netatalk/AppleVolumes.default"
#define SMBCONF "/etc/samba/smb.conf.drv"
#define AUTODRV "/etc/auto.drv";

char *owner = OWNER;
char *makefs = MAKEFS;
char *tunefs = TUNEFS;
char *fsoptions = FSOPTIONS;
char *applevolumes = APPLEVOLUMES;
char *smbconf = SMBCONF;
char *autodrv = AUTODRV;
char *dev_dir = DEV_DIR;
char *program;

void usage(void) {

	printf("Usage: %s\n", program);
	exit(1);
}

void initialize(int argc, char **argv) {

	char *v;

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
}

void setup_device(struct inotify_event *event) {

	pid_t pid;
	char path[PATH_LEN];
	char command[COMMAND_LEN];

/*
 * Fork a child process and return.  The child finishes the rest of this
 * function.
 */

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

/*
 * Child process.  Initialize the disk array element here.
 */

   system("ls -l /sys/block/sdl");
	if (!strstr(event->name, ".tmp")) {
		snprintf(path, PATH_LEN, "%s%s", DEV_DIR, event->name);
		sleep(UDEV_JITTER);
		if (!is_unpartitioned_scsi_disk(path)) {
			return;
		}
/*
	snprintf(command, COMMAND_LEN, 
			"perl /usr/local/sbin/auto_init_array_device %s", name);
	system(command);
*/
		printf("Found unitialized device: %s\n", event->name);
   	system("ls -l /dev/sd*");
      system("ls -l /sys/block/sdl");
	}
}

int main(int argc, char **argv) {

	int fd;
	int watch;
	char buf[BUF_LEN];
	char path[PATH_LEN];
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
	watch = inotify_add_watch(fd, DEV_DIR, IN_CREATE);
	if (watch < 0) {
		syslog(LOG_ERR, "Cannot add inotify watch: %s.", DEV_DIR);
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
