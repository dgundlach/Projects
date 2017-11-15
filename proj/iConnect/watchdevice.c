#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <wait.h>
#include <syslog.h>
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
			break; /* This will never get executed */
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

	if (fd = open(device, O_RDONLY|O_NOCTTY|O_NONBLOCK)) {
		close(fd);
	} else {
		dCleanUp(0);
	}
}

char *scsi_device(char *device) {

	int dnum;
	char *newdev = NULL;
	char *n, *d, c;

	d = device;
	if (!strncmp(d, "/dev/", 5)) {
		d += 5;
	}
	if ((*d++ == 's') && (*d++ == 'd')) {
		n = newdev = xmalloc(10);
		n += sprintf(newdev, "/dev/sd");
		c = *d++;
		if (islower(c)) {
			*n++ = c;
			dnum = (int)c - 'a';
			c = *d++;
			if (islower(c)) {
				*n++ = c;
				dnum = (dnum + 1) * 26 + (int)c - 'a';
				c = *d;
			}
			*n = c;
			if (c || (dnum > 255)) {
				free(newdev);
				newdev = NULL;
			}
		}
	}
	return newdev;
}

int main(int argc, char **argv) {

	char *device;
	char pidname[10];
	
	if (argc != 2) {
		exit(1);
	}
	if (device = scsi_device(argv[1])) {
		sprintf(pidname, "%s.pid", device + 5);
		dInitialize(argv[0], pidname, NULL, -1);
		dDaemonize();
		shSetHandlers(_SIGCHLD|_SIGHUP|_SIGTERM,
				      _SIGTSTP|_SIGTTOU|_SIGTTIN,
				      &signal_handler);
		while (1) {
			sleep(15);
			touchdevice(device);
		}
	} else {
		exit(1);
	}
}
