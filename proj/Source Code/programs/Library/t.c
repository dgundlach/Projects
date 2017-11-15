#include <stdio.h>
#include <string.h>
#include "Stack.h"
#include "HexDump.h"

int main(int argc, char **argv) {

    int i;
    void *buffer;
    int dsize;

    for (i=1; i<argc; i++) {
        printf("%s ", argv[i]);
        StackPush(argv[i], strlen(argv[i]) + 1);
    }
    printf("\n");
    if ((buffer = StackPeek(&dsize))) {
        HexDump(buffer, dsize, -1);
        StackShrink();
    }
    while ((buffer = StackPop(&dsize))) {
        HexDump(buffer, dsize, -1);
        free(buffer);
    }
    printf("\n");
    exit(0);
}
