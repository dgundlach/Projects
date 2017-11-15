#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <wait.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include "sighandler.h"
#include "daemonize.h"
#include "xalloc.h"

int  status;

void signal_handler(int signum) {

	int w;

	switch(signum) {
		case SIGHUP:
			break;
		case SIGTERM:
			dCleanUp(0);
			break;			// This will never get executed
		case SIGCHLD:
			w = wait(&status);
	}
}

void touchdevice(char *device) {

	int fd;

//
//  This routine is pretty simple.  It just opens the device node if it exists,
//  then closes it.  This is enough to trigger the kernel to refresh the
//  partition table if the device is a block device.  If the node is not able
//  to be opened, the program exits after cleaning up.
//

	if ((fd = open(device, O_RDONLY|O_NOCTTY|O_NONBLOCK)) != -1) {
		close(fd);
	} else {
		dCleanUp(0);
	}
}

int is_scsi_device(char *str) {

	register char *s = str + 7;

	if (!strncmp(str, "/dev/sd", 7) && islower(*s++)
			&& (!*s || (islower(*s) && !*++s)))
		return 1;
	return 0;
}

int main(int argc, char **argv) {

	char *p;
	int len;
	int interval = 15;
	int opt;
	char *pidname;
	char *device = NULL;

	while ((opt = getopt(argc, argv, "i:")) != -1) {
		switch (opt) {
			case 'i':
				interval = strtoul(optarg, &p, 10);
				if (*p) {
					exit(1);
				}
				if (interval < 15) {
					interval = 15;
				}
				break;
			default:
				exit(1);
		}
	}
	if ((optind + 1) != argc) {
		exit(1);
	}
	if (!strncmp(argv[optind], "sd", 2)) {
		device = malloc(16 + strlen(argv[optind]));
		sprintf(device, "/dev/%s", argv[optind]);
	} else {
		device = argv[optind];
	}
	if (is_scsi_device(device)) {
		dInitialize(argv[0], device + 5, "root", -1);
		dDaemonize();
		shSetHandlers(_SIGCHLD|_SIGHUP|_SIGTERM,
				      _SIGTSTP|_SIGTTOU|_SIGTTIN,
				      &signal_handler);
		while (1) {
			sleep(interval);
			touchdevice(device);
		}
	} else {
		exit(1);
	}
}
