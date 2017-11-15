#include <string.h>
#include <stdlib.h>
#include "command.h"
#include "splitargs.h"

int command(char *cmd, struct commands *c) {

    int argc = 0;
    int code = 0;
    int i = 0;
    char **argv;

    if ((argv = splitargs(cmd, ' ', &argc))) {
        for (i = 0; c[i].text; i++) {
            if (!strcasecmp(c[i].text, argv[0])) {
                break;
            }
        }
        code = c[i].fun(argc, argv);
        if (c[i].flush) {
            c[i].flush();
        }
        free(argv);
    }
    return code;
}

