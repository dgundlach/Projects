#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <libgen.h>

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
