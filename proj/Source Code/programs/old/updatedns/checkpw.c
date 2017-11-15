#define _XOPEN_SOURCE
#include <pwd.h>
#include <shadow.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include "checkpw.h"

#define	DAY			86400

int checkpw(char *login, char *password) {

    struct spwd *spw;
    int today;
    char *encrypted;

    today = time(NULL) / DAY;
    spw = getspnam(login);
    if (!spw) {
        return CHECKPW_NO_USER;
    }
    if ((spw->sp_expire > 0) && (spw->sp_expire < today)) {
        /* It's expired */
        return CHECKPW_PW_EXPIRED;
    }
    if (spw->sp_lstchg == 0) {
        /* Root expired it */
        return CHECKPW_PW_MANUALLY_EXPIRED;
    }
    if ((spw->sp_max >= 0) && (spw->sp_inact >= 0) &&
            (spw->sp_lstchg + spw->sp_max + spw->sp_inact < today)) {
        /* Password is too old */
        return CHECKPW_PW_TOO_OLD;
    }
    encrypted = crypt(password, spw->sp_pwdp);
    if (strcmp(encrypted,spw->sp_pwdp)) {
        return CHECKPW_PW_BAD;
    }
    return CHECKPW_PW_OK;
}
