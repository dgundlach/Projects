#include <stdlib.h>

#define CHECK_ARRAY_SIZE \
        if (args == maxargs) { \
            maxargs += 8; \
            if ((v = realloc(argv, maxargs * sizeof(char *)))) { \
                argv = v; \
            } else { \
                if (argv) { \
                    free(argv); \
                } \
                return NULL; \
            } \
        }

char **SplitArgs(char *cmd, int *argc, char **argv, int (*issep)(char)) {

    char **v;
    char *s;
    int len = 0;
    int args = 0;
    int maxargs;

    maxargs = (argc) ? *argc : 0; 
    s = cmd;
    while (*s) {
        if (*s == '\r' || *s == '\n') {
            *s = '\0';
            break;
        }
        if (issep(*s)) {
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

