#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "hexdump.h"
#include "split.h"

int main (int argc, char **argv) {

    int i = 0;
    struct splitstr_t *sa;
    char *input;

    if (argc != 2) exit(1);
    input = argv[1];
    sa = split(NULL, input, ":");
    while (i <= sa->c - 1) {
        printf("%02i :%s:\n", i, sa->s[i]);
        i++;
    }
    i = 0;
    while (sa->s[i]) {
        printf("%02i :%s:\n", i, sa->s[i]);
        i++;
    }
    i = 0;
    sa = psplit(sa, input, ":");
    while (i <= sa->c - 1) {
        printf("%02i :%s:\n", i, sa->s[i]);
        i++;
    }
    i = 0;
    while (sa->s[i]) {
        printf("%02i :%s:\n", i, sa->s[i]);
        i++;
    }
    free(sa);
    exit(0);


}
