#include <sys/types.h>
#include <string.h>
#include "prealloc.h"

int str_ready(char **str, size_t *oldsize, size_t newsize, int pad) {

    int ns;
    char *buff;

    if (newsize >= *oldsize) {
        ns = newsize;
        if ((buff = prealloc(*str, &ns, pad))) {
            *oldsize = ns;
            *str = buff;
        } else {
            return 0;
        }
    }
    return 1;
}

int rstrncpy(char **dest, size_t *dsize, size_t offset, const char *src, 
                                                        size_t ssize) {

    int i;

    if (!str_ready(dest, dsize, offset + ssize, 1)) {
        return -1;
    }
    for (i = 0; i < ssize; i++) {
        *dest[offset++] = src[i];
    }
    *dest[offset] = '\0';
    return ssize;
}

int rstrcpy(char **dest, size_t *dsize, size_t offset, const char *src) {

    int i, off;

    off = offset;
    for (i = 0; src[i]; i++) {
        if (off >= *dsize && !str_ready(dest, dsize, off + strlen(src + 1) + 1, 1)) {
            return -1;
        }
        *dest[off++] = src[i];
    }
    *dest[off] = '\0';
    return i;
}

int rstrncpye(char **dest, size_t *dsize, size_t offset, 
                       const char *src, size_t ssize, 
                       size_t (*escape)(char *, const char *, size_t)) {


    if (!str_ready(dest, dsize, offset + (ssize << 1), 1)) {
        return -1;
    }
    return escape(*dest + offset, src, ssize);
}

int rstrcpye(char **dest, size_t *dsize, size_t offset, const char *src,
                       size_t (*escape)(char *, const char *, size_t)) {

    return rstrncpye(dest, dsize, offset, src, strlen(src), escape);
}
