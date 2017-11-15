#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <libgen.h>
#include <unistd.h>

int mkdirp(const char *pathname, mode_t mode) {

	char pn[strlen(pathname) + 1];
	char *dn;
	struct stat st;
	int rv = 0;

//
//  Check if the parent directory exists.  If not, make a recursive call to this
//  subroutine.
//

	strcpy(pn, pathname);
	dn = dirname(pn);
	if (stat(dn, &st)) {
		rv = mkdirp(dn, mode);
    }

//
//  Even if the path exists, try to create it so that the return values are
//  properly set.  But not if we were not able to create the parent.
//

	if (!rv) {
		rv = mkdir(pathname, mode);
	}
	return rv;
}

int mkdirpo(const char *pathname, mode_t mode, uid_t uid, gid_t gid) {

	char pn[strlen(pathname) + 1];
	char *dn;
	struct stat st;
	int rv = 0;

//
//  Check if the parent directory exists.  If not, make a recursive call to this
//  subroutine.
//

	strcpy(pn, pathname);
	dn = dirname(pn);
	if (stat(dn, &st)) {
		rv = mkdirpo(dn, mode, uid, gid);
    }

//
//  Even if the path exists, try to create it so that the return values are
//  properly set.  But not if we were not able to create the parent.
//

	if (!rv) {
		if (!(rv = mkdir(pathname, mode)) && !getuid() && (uid || gid)) {

//
//  If we're the root user, and the directory is to be owned by a different
//  user, set the ownership.
//

			chown(pathname, uid, gid);
		}
	}
	return rv;
}
