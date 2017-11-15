#include <string.h>
#include <strings.h>
#include <stdio.h>

int strtailcmp(const char *s, char *t, int case_sensitive) {

    int offset;

    offset = strlen(s) - strlen(t);
    if (offset < 0) {
	offset = 0;
    }
    if (case_sensitive) {
	return strcmp(s + offset, t);
    } else {
	return strcasecmp(s + offset, t);
    }
}

int main(int argc, char **argv) {

    int i;

    i = strtailcmp(argv[1], argv[2], 0);
    printf("%i\n", i);
    i = strtailcmp(argv[1], argv[2], 1);
    printf("%i\n", i);
}
