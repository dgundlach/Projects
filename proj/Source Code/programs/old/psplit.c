#include <string.h>
#include "split.h"

struct splitstr_t *psplit (struct splitstr_t *sai, char *str, char *delim) {

    struct splitstr_t *sa;
    int i;
    char *s1, *s2, *d;
    char c;

    if (!(sa = split_ready(sai, strlen(str)))) return NULL;
    i = 1;
    s1 = str;
    s2 = sa->s[0];
    while (c = *s1++) {
        d = delim;
        while (*d) {
	    if (c == *d) {
		c = '\0';
		sa->s[i++] = s2 + 1;
		break;
	    }
	    d++;
	}
	*s2++ = c;
    }
    *s2 = '\0';
    sa->s[i] = NULL;
    sa->c = i;
    return sa;
}
