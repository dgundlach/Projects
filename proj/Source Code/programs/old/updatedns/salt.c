#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <time.h>
#include "salt.h"

static char _salt_string[] = "$1$........";
static unsigned int _salt_seed = 0;

char *salt(int salttype) {

    char *s;
    static char *b64  = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
			"abcdefghijklmnopqrstuvwxyz0123456789./";

    if (!_salt_seed) {
	_salt_seed = (getuid() << 7) ^ (getpid() << 4) ^ time(NULL);
	srand(_salt_seed);
    }

    s = _salt_string + 3;
    while (*s) {
        *s++ = b64[(int)(64.0 * rand() / (RAND_MAX + 1.0))];
    }
    if (salttype == SALT_DES) {
	return _salt_string + 9;
    }
    return _salt_string;
}
