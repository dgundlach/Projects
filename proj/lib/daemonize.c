#include <sys/types.h>
#include <sys/stat.h>
#include <limits.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <pwd.h>
#include <libgen.h>
#include <syslog.h>
#include "openp.h"
#include "xalloc.h"

#define PID_PATH "/var/run"

//
//  These are declared as static so that their scope is only defined in this
//  file.
//

static int   _log_events		 = 0;
static char  *_program_name		 = NULL;
static char  *_pid_path			 = NULL;
static char  *_pid_file			 = NULL;
static uid_t _uid				 = 0;
static gid_t _gid				 = 0;
static void (*dPreCleanUp)(void) = NULL;

void dCleanUp(int exitstatus) {

	int i;

//
//  Ignore any signals to terminate.
//

	signal(SIGTERM, SIG_IGN);

//
//  Call the clean up routine that's defined by the program, if any.
//

	if (dPreCleanUp) {
		dPreCleanUp();
	}

//
//  The rest is pretty generic.  It stops any logging, closes all files,
//  deletes the pid file, kills any children, and exits.
//

	if (_log_events) {
		syslog(LOG_INFO, "Exiting.");
		closelog();
	}
	for (i=getdtablesize(); i>=0; --i) {
		close(i);
	}
	unlink(_pid_file);
	kill(0, SIGTERM);
	exit(exitstatus);
}

void dXallocFatal(char *message) {

	if (_log_events) {
		syslog(LOG_ERR, message);
	}
	dCleanUp(-1);
}

void dInitialize(const char *name, const char *pidname,
				 const char *user, int facility) {

	char *nc;
	struct passwd *pwent;
	uid_t uid;

	xalloc_fatal_call = &dXallocFatal;

//
//  Set the program name.
//

	nc = xstrdup((char *)name);
	_program_name = basename(nc);

//
//  Set the log facility.
//

	if (facility >= 0) {
		_log_events = 1;
		openlog(_program_name, LOG_PID, facility);
		syslog(LOG_INFO, "Starting.");
	}

//
//  What user started the program?
//

	uid = _uid = getuid();
	_gid = getgid();
	if (!uid && user && strcmp(user, "root")) {
		if (!(pwent = getpwnam(user))) {
			if (_log_events) {
				syslog(LOG_ERR, "No such user: %s", user);
			}
			dCleanUp(-1);
		}
		_uid = pwent->pw_uid;
		_gid = pwent->pw_gid;
	}

//
//  If the calling program set an absolute path to the pid name, use it.
//

	if (pidname && *pidname == '/') {
		_pid_file = xstrdup((char *)pidname);
	} else {
		if (!_pid_path) {

//
//  Figure out the pid path.  If the starting user not root, the pid path
//  will have to be in the home directory of the user that started it.
//

			if (uid) {
				if (!(pwent = getpwuid(uid))) {
					if (_log_events) {
						syslog(LOG_ERR, "No such uid: %s", uid);
					}
					dCleanUp(-1);
				}
				_pid_path = xmalloc(strlen(pwent->pw_dir)
					+ strlen(_program_name) + 16);
				sprintf(_pid_path, "%s/.run", pwent->pw_dir);

//
//  The uid of the program will not be changed, as the starting user is not
//  root.  Clear it.  The gid can be changed if the user is part of that group.
//

				_uid = 0;
			} else {

//
//  The starting user is root.  We can use the default path for the pid file.
//

				_pid_path = xmalloc(strlen(_program_name) + 16);
				strcpy(_pid_path, PID_PATH);
			}
		} // else {

//
//  If the pid path was already set, we will assume that it's writeable by
//  the starting user, so we do nothing here.
//

//		}

		_pid_file = xmalloc(strlen(name) + strlen(pidname)
										 + strlen(_pid_path)
										 + 16);
		if (pidname) {

//
//  If a pid name was passed, the pid should be created in a directory named
//  after the calling program.  Alter the pid path with this information.
//

			sprintf(_pid_file, "%s/%s/%s.pid", 
							   _pid_path,
							   _program_name,
							   pidname);
			sprintf(_pid_path + strlen(_pid_path), "/%s", _program_name);
		} else {
			if (_uid) {

//
//  The program was started by root, but will run as non-root.  Put the pid
//  file in a directory named after the program, and set the pid path
//  accordingly.
//

				sprintf(_pid_file, "%s/%s/%s.pid", 
								   _pid_path,
								   _program_name,
								   _program_name);
				sprintf(_pid_path + strlen(_pid_path), "/%s", _program_name);
			} else {

//
//  The user that started the program has write access to the pid file, so
//  we don't have to place it in a special directory.
//

				sprintf(_pid_file, "%s/%s.pid", 
								   _pid_path,
								   _program_name);
			}
		}
	}
}

void dDaemonize(void) {

	int i, pid, pf;
	char str[10];

	if (getppid() == 1) return;
	pid = fork();
	switch(pid) {
		case -1:
			exit(1);
		case 0:        // Child process continues
			break;
		default:       // Parent process returns
			exit(0);
	}
	setsid();

//
//  Close all open files.
//

	for (i=getdtablesize(); i>=0; --i) {
		close(i);
	}

//
//  Redirect stdin, stdout and stderr to /dev/null.
//

	i=open("/dev/null", O_RDWR);
	dup(i);
	dup(i);

//
//  Open the pid file, and create the path if necessary.
//

	if ((pf = openpo(_pid_file, O_RDWR|O_CREAT, 0640, 0750,
					 _uid, _gid)) < 0) {
		if (_log_events) {
			syslog(LOG_ERR, "Cannot open PID file.");
		}
		dCleanUp(-1);
	}

//
//  Make sure that we have an exclusive lock on the pid file.
//

	if (lockf(pf, F_TLOCK, 0) < 0) {
		syslog(LOG_ERR, "Cannot get exclusive lock.");
		dCleanUp(-1);
	}
	sprintf(str, "%d\n", getpid());
	write(pf, str, strlen(str));

//
//  Change to the new gid and uid if necessary.
//

	if (_gid) {
		setgid(_gid);
	}
	if (_uid) {
		setuid(_uid);
	}
}

//
//  The following routines provide read access to the internal variables
//  used by the main routines.
//
//      _log_events
//      _program_name
//      _pid_path
//      _pid_file
//

int dGetLogStatus(void) {

	return _log_events;
}

char *dGetProgramName(void) {

	return _program_name;
}

char *dGetPidPath(void) {

	return _pid_path;
}

void dSetPidPath(const char *path) {

	if (path) {
		_pid_path = xstrdup((char *)path);
	}
}

char *dGetPidFile(void) {

	return _pid_file;
}
