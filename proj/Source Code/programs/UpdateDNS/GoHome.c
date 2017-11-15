#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#include "GoHome.h"

int GoHome(void) {

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
