#define _XOPEN_SOURCE
#include <stdio.h>
#include <sys/types.h>
#include <pwd.h>
#include <shadow.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <errno.h>
#define _CHECKPW_SOURCE
#include "checkpw.h"

#define	DAY 86400

char *checkpw_errstr[] = {
		"password correct",
		"user does not exist",
		"password expired",
		"password expired by root",
		"password too old",
		"password disabled",
		"password incorrect",
		NULL
};

static char *legal =
	"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789./$";

int checkpw(char *login, char *password, getuser_f getuser,
										 getuser_f getshadow) {

	struct passwd *pwd;
	struct spwd *spw;
	int today;
	char *storedpw;
	char *encrypted;
	getuser_f passwdf = (void *)&getpwnam;
	getuser_f shadowf = (void *)&getspnam;

	if (getuser) {
		passwdf = getuser;
	}
	if (getshadow) {
		shadowf = getshadow;
	}
	if (!(pwd = passwdf(login))) {
		return CHECKPW_NO_USER;
	}
	storedpw = pwd->pw_passwd;
	if (strlen(storedpw) < 10) {
		today = time(NULL) / DAY;
		if (!(spw = shadowf(login))) {
			return CHECKPW_PW_DISABLED;
		}
		if ((spw->sp_expire > 0) && (spw->sp_expire < today)) {
			// It's expired
			return CHECKPW_PW_EXPIRED;
		}
		if (spw->sp_lstchg == 0) {
			// Root expired it
			return CHECKPW_PW_MANUALLY_EXPIRED;
		}
		if ((spw->sp_max >= 0) && (spw->sp_inact >= 0) &&
				(spw->sp_lstchg + spw->sp_max + spw->sp_inact < today)) {
			// Password is too old
			return CHECKPW_PW_TOO_OLD;
		}
		storedpw = spw->sp_pwdp;
	}
	if (strspn(spw->sp_pwdp, legal) != strlen(spw->sp_pwdp)) {
		// Bad character in encrypted password.  Disabled.
		return CHECKPW_PW_DISABLED;
	}
	encrypted = crypt(password, storedpw);
	if (strcmp(encrypted, storedpw)) {
		return CHECKPW_PW_BAD;
	}
	return CHECKPW_PW_OK;
}

extern struct passwd *fgetpwent(FILE *);

char *alt_passwd_file = NULL;
char *alt_shadow_file = NULL;

struct passwd *alt_getpwnam(char * login) {

	static FILE *passwdf = NULL;
	static char lastlogin[256];
	static struct passwd *pwd;

	if (!alt_passwd_file) {
		return NULL;
	}
	if (!passwdf) {
		if (!(passwdf = fopen(alt_passwd_file, "r"))) {
			return NULL;
		}
	}
	if (!*lastlogin) {
		if (!(pwd = fgetpwent(passwdf))) {
			fclose(passwdf);
			passwdf = NULL;
			return NULL;
		}
		strncpy(lastlogin, pwd->pw_name, sizeof(lastlogin));
	}
	while (strcmp(pwd->pw_name, login)) {
		if (!(pwd = fgetpwent(passwdf))) {
			fseek(passwdf, 0, 0);
			if (!(pwd = fgetpwent(passwdf))) {
				fclose(passwdf);
				passwdf = NULL;
				*lastlogin = '\0';
				return NULL;
			}
		}
		if (!strcmp(pwd->pw_name, lastlogin)) {
			return NULL;
		}
	}
	strncpy(lastlogin, pwd->pw_name, sizeof(lastlogin));
	return pwd;
}

struct spwd *alt_getspnam(char * login) {

	static FILE *shadowf = NULL;
	static char lastlogin[256];
	static struct spwd *spw;

	if (!alt_shadow_file) {
		return NULL;
	}
	if (!shadowf) {
		if (!(shadowf = fopen(alt_shadow_file, "r"))) {
			return NULL;
		}
	}
	if (!*lastlogin) {
		if (!(spw = fgetspent(shadowf))) {
			fclose(shadowf);
			shadowf = NULL;
			return NULL;
		}
		strncpy(lastlogin, spw->sp_namp, sizeof(lastlogin));
	}
	while (strcmp(spw->sp_namp, login)) {
		if (!(spw = fgetspent(shadowf))) {
			fseek(shadowf, 0, 0);
			if (!(spw = fgetspent(shadowf))) {
				fclose(shadowf);
				shadowf = NULL;
				*lastlogin = '\0';
				return NULL;
			}
		}
		if (!strcmp(spw->sp_namp, lastlogin)) {
			return NULL;
		}
	}
	strncpy(lastlogin, spw->sp_namp, sizeof(lastlogin));
	return spw;
}
