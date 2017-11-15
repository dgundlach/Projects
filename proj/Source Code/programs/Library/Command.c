#include <string.h>
#include <stdlib.h>
#include "Command.h"
#include "SplitArgs.h"

int Command(char *cmd, struct commands *c) {

    int argc = 0;
    int code = 0;
    int i = 0;
    char **argv;

    if ((argv = SplitArgs(cmd, ' ', &argc))) {
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
