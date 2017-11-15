#include <pwd.h>
#include "gohome.h"

int gohome(void) {

    struct passwd *pw;

    pw = getpwuid(getuid());
    if (!pw) {
	return GOHOME_NO_PASSWORD_ENTRY;
    }
    if (chdir(pw->pw_dir) == -1) { 
	return GOHOME_CANT_GO_HOME;
    }
    return GOHOME_OK;
}
