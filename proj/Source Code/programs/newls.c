#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

int main (int argc, char **argv) {

    DIR *d;
    struct dirent *de;
    struct stat st;

    if (argc != 2) {
        printf("Usage: %s [dir]\n", argv[0]);
        exit(1);
        printf("Here\n");
    }
    if (! (d = opendir(argv[1]))) {
        printf("No such directory: %s\n", argv[1]);
        exit(1);
    }
    while (de = readdir(d)) {
        if (!strcmp(de->d_name, ".") || !strcmp(de->d_name, "..")) {
            break;
        }
        stat(de->d_name, &st);
        if (st.st_mode && S_IFREG) {     /* Regular file */
            printf("%i:%s\n", st.st_size, de->d_name);
        }
    }
    closedir(d);
    exit(0);
}
