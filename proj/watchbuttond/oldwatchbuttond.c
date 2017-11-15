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

// Permanent defines and global variables go here.
#define LOG_FACILITY LOG_LOCAL6
int  log_events     = 0;
char *program       = NULL;
char *lock_file     = NULL;

void set_program_name(char *name) {

	program = basename(name);
	lock_file = malloc(strlen(program) + 16);
	sprintf(lock_file, "/var/lock/%s/pid", program);
}

void clean_up(void) {

	int i;

	if (log_events) {
		syslog(LOG_INFO, "Exiting.");
		closelog();
	}
	for (i=getdtablesize(); i>=0; --i) close(i);
	unlink(lock_file);
	kill(0, SIGTERM);
	exit(0);
}

void daemonize(void) {

	int i, pid, pf;
	char str[10];

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
	if (log_events) {
		openlog(program, LOG_PID, LOG_FACILITY);
		syslog(LOG_INFO, "Starting.");
	}
	setsid();
	for (i=getdtablesize(); i>=0; --i) close(i);
	i=open("/dev/null", O_RDWR);
	dup(i);
	dup(i);
	if ((pf = open(lock_file, O_RDWR | O_CREAT, 0640)) < 0) {
		if (log_events) {
			syslog(LOG_ERR, "Cannot open lock file.");
		}
		clean_up();
	}
	if (lockf(pf, F_TLOCK, 0) < 0) {
		syslog(LOG_ERR, "Cannot get exclusive lock.");
		clean_up();
	}
	sprintf(str, "%d\n", getpid());
	write(pf, str, strlen(str));
}

void set_user(char *user) {

	struct passwd *pwent;

	if (strcmp(user, "root") && !getuid()) {
		if (!(pwent = getpwnam(user))) {
			if (log_events) {
				syslog(LOG_ERR, "No such user: %s", user);
			}
			clean_up();
		}
		setgid(pwent->pw_gid);
		setuid(pwent->pw_uid);
	}
}

// ----------------------- End of template ---------------------

#define NUM_EVENTS 64   /* Number of events to read in at once */
int  status;
char *device        = NULL;
char *exec_pressed  = NULL;
char *exec_released = NULL;
char *user_pressed  = "root";
char *user_released = "root";

void signal_handler(int signum) {

	int w;

	switch(signum) {
		case SIGHUP:
			break;
		case SIGTERM:
			clean_up();
			break; /* This will never get executed */
		case SIGCHLD:
			w = wait(&status);
	}
}

void set_signal_handlers(void) {

	signal(SIGCHLD, &signal_handler);
	signal(SIGTSTP, SIG_IGN);
	signal(SIGTTOU, SIG_IGN);
	signal(SIGTTIN, SIG_IGN);
	signal(SIGHUP, &signal_handler);
	signal(SIGTERM, &signal_handler);
}

void usage() {
	fprintf(stderr, "Usage: %s [-l] <[-p <pressed>] [-r <released>]>\n"
			"       [-P <pressed user>] [-R <released user>] device\n",
			program);
	exit(1);
}

void process_args(int argc, char **argv) {

	int c;

	while ((c = getopt(argc, argv, "lp:P:r:R:")) != -1) {
		switch (c) {
			case 'l':
				log_events = 1;
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
			device = malloc(strlen(argv[optind++]) + 16);
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
}

void call_program(char *user, char *command) {

	int pid = 8675309;  /* set to some bugus value in case we don't fork :) */
	int rc;

	set_user(user);
	if (pid = fork()) return;
	rc = system(command);
	if (log_events) {
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

	set_program_name(argv[0]);
    process_args(argc, argv);
	daemonize();
	set_signal_handlers();

	if (!(fd = open(device, O_RDONLY | O_EXCL))) {
		syslog(LOG_ERR, "Cannot gain exclusive lock.");
		clean_up();
	}
	while (1) {
		rb=read(fd, ev, sizeof(struct input_event) * NUM_EVENTS);
		if (rb < (int) sizeof(struct input_event)) {
			if (log_events) {
				syslog(LOG_ERR, "Short read button device.");
			}
			clean_up();
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
