#define _XOPEN_SOURCE
#include <unistd.h>
#include <stdio.h>

int main (int argc, char **argv) {

    char salt[] = "$1$........";
    int i;

    if (argc != 3) exit(1);
    for (i = 0; i < 8; i++) {
        salt[3 + i] = argv[2][i];
    }
    printf("curly.msl.net,%s\n", crypt(argv[1], salt));
}
