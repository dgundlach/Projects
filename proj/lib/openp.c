#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <libgen.h>
#include <unistd.h>
#include <errno.h>
#include "mkdirp.h"

int openp(const char *path, int flags, mode_t file_mode, mode_t dir_mode) {

	char pn[strlen(path) + 1];
	char *dn;
	int fd;

//
//  The simplest way to check if a file is writeable is to open it for writing.
//  If the flags were set to O_RDONLY, errno will be set by the call to open.
//  Set the O_CREAT flag for the calling program, though.
//

	if ((fd = open(path, flags|O_CREAT, file_mode)) < 0) {

//
//  The open failed.  The full path may not exist, so try to create it.
//

		strcpy(pn, path);
		dn = dirname(pn);
		if (!mkdirp(dn, dir_mode)) {

//
//  We've created all the parent directories, so try to create the file again.  It
//  should always succeed.
//

			fd = open(path, flags|O_CREAT, file_mode);
		}
	}
	return fd;
}

int openpo(const char *path, int flags, mode_t file_mode, mode_t dir_mode, uid_t uid, gid_t gid) {

	char pn[strlen(path) + 1];
	char *dn;
	int fd;

//
//  The simplest way to check if a file is writeable is to open it for writing.
//  If the flags were set to O_RDONLY, errno will be set by the call to open.
//  Set the O_CREAT flag for the calling program, though.
//

	if ((fd = open(path, flags|O_CREAT, file_mode)) < 0) {

//
//  The open failed.  The full path may not exist, so try to create it.
//

		strcpy(pn, path);
		dn = dirname(pn);
		if (!mkdirpo(dn, dir_mode, uid, gid)) {

//
//  We've created all the parent directories, so try to create the file again.  It
//  should always succeed.
//

			fd = open(path, flags|O_CREAT, file_mode);
		}
	}

//
//  If we were successful, and we're the root user, set the ownership on the file 
//  if it's for another user.
//

	if ((fd >= 0) && !getuid() && (uid || gid)) {
		chown(path, uid, gid);
	}
	return fd;
}
