#include <stdio.h>

void chomp(char *str) {

    char *s;

    s = str;
    do {
	switch (*s) {
	case '\r':
	case '\n':
	    *s = '\0';
	    break;
	}
    } while (*s++);
}
