#include <stdlib.h>
#include "split.h"

struct splitstr_t *split_ready (struct splitstr_t *sai, int nchars) {

    int len;
    struct splitstr_t *sa;
    int asiz;
    char *s;

    len = nchars + 1;
    asiz = len + len * sizeof(char *) + sizeof(struct splitstr_t);
    sa = sai;
    if (!sai || (sai && sai->a < asiz)) {
	if (!(sa = realloc(sai, asiz))) return NULL;
	sa->a = asiz;
    }
    s = (char *) sa + sizeof(struct splitstr_t);
    sa->s = (char **) s;
    sa->s[0] = (char *) (sa->s) + (len * sizeof(char *));
    return sa;
}
