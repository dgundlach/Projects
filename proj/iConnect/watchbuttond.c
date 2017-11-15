#include <linux/limits.h>
#include <linux/input.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <libgen.h>
#include <signal.h>
#include <syslog.h>
#include <getopt.h>
#include <pwd.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include "sighandler.h"
#include "daemonize.h"
#include "xalloc.h"

#define LOG_FACILITY LOG_LOCAL6
#define DEFAULT_CONF_DIR "/etc/watchbuttond"
#define NUM_EVENTS 64   /* Number of events to read in at once */
int  status;
char *device        = NULL;
char *exec_pressed  = NULL;
char *exec_released = NULL;
char *user_pressed  = "root";
char *user_released = "root";
char *conf_dir		= DEFAULT_CONF_DIR;

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

void usage() {
	fprintf(stderr, "Usage: %s [-l] <[-p <pressed>] [-r <released>]>\n"
			"       [-P <pressed user>] [-R <released user>] device\n",
			dGetProgramName());
	exit(1);
}

void initialize(int argc, char **argv) {

	int c;
	int facility = -1;

	while ((c = getopt(argc, argv, "c:lp:P:r:R:")) != -1) {
		switch (c) {
			case 'c':
				conf_dir = optarg;
				break;
			case 'l':
				facility = LOG_FACILITY;
				break;
			case 'p':
				exec_pressed = optarg;
				break;
			case 'P':
				user_pressed = optarg;
				break;
			case 'r':
				exec_released = optarg;
				break;
			case 'R':
				user_released = optarg;
				break;
			default:
				usage();
		}
	}
	if (!exec_pressed && !exec_released) {
		fprintf(stderr, "-p or -r not specified.\n");
		usage();
	}
	if (optind < argc) {
		if (strchr(argv[optind], '/')) {
			device = argv[optind++];
		} else {
			device = xmalloc(strlen(argv[optind++]) + 16);
			sprintf(device, "/dev/input/%s", argv[optind++]);
		}
	} else {
		fprintf(stderr, "No device specified.\n");
		usage();
	}
	if (optind < argc) {
		fprintf(stderr, "Too many arguments.\n");
		usage();
	}
	dInitialize(argv[0], NULL, NULL, facility);
	dDaemonize();
	shSetHandlers(_SIGCHLD|_SIGHUP|_SIGTERM,
			      _SIGTSTP|_SIGTTOU|_SIGTTIN,
			      &signal_handler);
}

void call_program2(short event, short type, short code, int value) {

	int pid = 8675309;
	int fd;
	int br;
	int rc;
	char path[PATH_MAX];
	char val[256];
	char user[256] = "root";
	struct passwd *pwent;
	
	if (pid = fork()) return;
	snprintf(path, PATH_MAX, "%s/user-event%d", conf_dir, event);
	if (fd = open(device, O_RDONLY)) {
		br = read(fd, val, 256);
		close(fd);
		if (br) {
			if (pwent = getpwnam(user)) {
				setgid(pwent->pw_gid);
				setuid(pwent->pw_uid);
			} else {
				if (dGetLogStatus()) {
					syslog(LOG_WARNING, "User: %s does not exist in %s", val, path);
				}
				if (pid) return;
				exit(0);
			}
		}
	}
	snprintf(val, 256, "%d", type);
	setenv("EVENT_TYPE", val, 1);
	snprintf(val, 256, "%d", code);
	setenv("EVENT_CODE", val, 1);
	snprintf(val, 256, "%i", value);
	setenv("EVENT_VALUE", val, 1);
	if (value) {
		snprintf(path, PATH_MAX, "%s/pressed-event%d", conf_dir, event);
	} else {
		snprintf(path, PATH_MAX, "%s/released-event%d", conf_dir, event);
	}
	rc = system(path);
	if (dGetLogStatus()) {
		syslog(LOG_INFO, "Executed: %s, Return: %d", path, rc);
	}
	if (pid) return;
	exit(0);
}

void set_user(char *user) {

    struct passwd *pwent;

    if (strcmp(user, "root") && !getuid()) {
        if (!(pwent = getpwnam(user))) {
            if (dGetLogStatus()) {
                syslog(LOG_ERR, "No such user: %s", user);
            }
            dCleanUp(-1);
        }
        setgid(pwent->pw_gid);
        setuid(pwent->pw_uid);
    }
}

void call_program(char *user, char *command) {

	int pid = 8675309;  /* set to some bugus value in case we don't fork :) */
	int rc;

	if (pid = fork()) return;
	set_user(user);
	rc = system(command);
	if (dGetLogStatus()) {
		syslog(LOG_INFO, "Executed: %s, Return: %d", command, rc);
	}
	if (pid) return;
	exit(0);
}

int main(int argc, char **argv) {

/* how many bytes were read */
	size_t rb;
/* the events (up to 64 at once) */
	struct input_event ev[NUM_EVENTS];
	int fd;
	int event;
	int i;

	initialize(argc, argv);
	if ((fd = open(device, O_RDONLY | O_EXCL)) < 0) {
		if (dGetLogStatus()) {
			syslog(LOG_ERR, "Cannot open device %s.", device);
		}
		dCleanUp(-1);
	}
	while (1) {
		rb=read(fd, ev, sizeof(struct input_event) * NUM_EVENTS);
		if (rb < (int) sizeof(struct input_event)) {
			if (dGetLogStatus()) {
				syslog(LOG_ERR, "Short read button device.");
			}
			dCleanUp(-1);
		}
		for (event = 0; event < NUM_EVENTS; event++) {
			if (ev[event].type) {
				if (ev[event].value) {
					if (exec_pressed) {
						call_program(user_pressed, exec_pressed);
					}
				} else {
					if (exec_released) {
						call_program(user_released, exec_released);
					}
				}
			}
		}
	}
}
