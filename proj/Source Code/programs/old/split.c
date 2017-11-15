#include <string.h>
#include "split.h"

struct splitstr_t *split (struct splitstr_t *sai, char *str, char *delim) {

    struct splitstr_t *sa;
    int len;
    int i;
    char *s1, *s2, *d;
    char c;

    if (!(sa = split_ready(sai, strlen(str)))) return NULL;
    i = 0;
    s1 = str;
    s2 = sa->s[0];
    sa->s[0] = NULL;
    len = 0;
    while (c = *s1++) {
        d = delim;
        while (*d) {
            if (c == *d) {
                c = '\0';
                if (len) {
                    i++;
                    len = 0;
                }
                break;
	    }
	    d++;
	}
        if (c) {
            len++;
            if (len == 1) {
                sa->s[i] = s2;
            }
	}
	*s2++ = c;
    }
    *s2 = '\0';
    if (sa->s[i]) {
	sa->s[++i] = NULL;
    }
    sa->c = i;
    return sa;
}
