#include <stdlib.h>

#define CHECK_ARRAY_SIZE \
	if (args == maxargs) { \
	    maxargs += 10; \
	    if ((v = realloc(argv, maxargs * sizeof(char *)))) { \
		argv = v; \
	    } else { \
		if (argv) { \
		    free(argv); \
		} \
		return NULL; \
	    } \
	}

char **splitargs(char *cmd, char sep, int *argc) {

    char **v, **argv = NULL;
    char *s;
    int len = 0;
    int args = 0;
    int maxargs = 0;

    s = cmd;
    while (*s) {
	if (*s == '\r' || *s == '\n') {
	    *s = '\0';
	    break;
	}
	if (*s == sep) {
	    *s = '\0';
	    len = 0;
	} else {
	    if (!len++) {
		CHECK_ARRAY_SIZE;
		argv[args++] = s;
	    }
	}
	s++;
    }
    if (args) {
	CHECK_ARRAY_SIZE;
	argv[args] = NULL;
    }
    if (argc) {
	*argc = args;
    }
    return argv;
}
