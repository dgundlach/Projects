#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "rsprintf.h"
#include "rstrcpy.h"
#include "stra.h"


int StrA_ready(StrA *str, size_t size) {

    return str_ready(&str->s, &str->a, str->l + size, 0);
}

StrA *StrA_new(size_t size) {

    StrA *n;

    if (!(n = malloc(sizeof(StrA)))) {
        return NULL;
    }
    bzero((void *)n, sizeof(StrA));
    if (size) {
        if (StrA_ready(n, size)) {
            return n;
        } else {
            return NULL;
        }
    }
    return n;
}

void StrA_destroy(StrA *str) {

    if (str->s) {
        free(str->s);
    }
    free(str);
}

int StrA_catc(StrA *str, char c) {

    if (str_ready(&str->s, &str->a, str->l + 1, 0)) {
        str->s[str->l++] = c;
        return 1;
    }
    return 0;
}

int StrA_copyc(StrA *str, char c) {

    str->l = 0;
    return StrA_catc(str, c);
}

int StrA_dupc(StrA *str, size_t count) {

    int i;
    char c;

    if (!str->l) {
        return 0;
    }
    c = str->s[str->l - 1];
    count += str->l;
    if (str_ready(&str->s, &str->a, count, 1)) {
        for (i = str->l; i < count; i++) {
            str->s[i] = c;
        }
        return 1;
    }
    return 0;
}

int StrA_0(StrA *str) {

    return StrA_catc(str, '\0');
}

int StrA_copyb(StrA *str, unsigned char *buf, size_t bsize) {

    if (str_ready(&str->s, &str->a, bsize, 1)) {
        memcpy(str->s, buf, bsize);
        str->l = bsize;
        return 1;
    }
    return 0;
}

int StrA_catb(StrA *str, unsigned char *buf, size_t bsize) {

    if (str_ready(&str->s, &str->a, str->l + bsize, 1)) {
        memcpy(str->s + str->l, buf, bsize);
        str->l += bsize;
        return 1;
    }
    return 0;
}

int StrA_copy(StrA *dest, StrA *src) {

    int r;

    if ((r = rstrncpy(&dest->s, &dest->a, 0, src->s, src->l)) >= 0) {
        dest->l = r;
        return 1;
    }
    return 0;
}

int StrA_cat(StrA *dest, StrA *src) {

    int r;

    if ((r = rstrncpy(&dest->s, &dest->a, dest->l, src->s, src->l)) >= 0) {
        dest->l += r;
        return 1;
    }
    return 0;
}

int StrA_copye(StrA *dest, StrA *src,
                       size_t (*escape)(char *, const char *, size_t)) {

    int r;
    if ((r = rstrncpye(&dest->s, &dest->a, 0, src->s, src->l, escape)) >= 0) {
        dest->l = r;
        return 1;
    }
    return 0;
}

int StrA_cate(StrA *dest, StrA *src,
                       size_t (*escape)(char *, const char *, size_t)) {

    int r;
    if ((r = rstrncpye(&dest->s, &dest->a, dest->l, src->s, src->l, escape)) >= 0) {
        dest->l = r;
        return 1;
    }
    return 0;
}

int StrA_copys(StrA *dest, char *src) {

    int r;

    if ((r = rstrncpy(&dest->s, &dest->a, 0, src, strlen(src))) >= 0) {
        dest->l = r;
        return 1;
    }
    return 0;
}

int StrA_cats(StrA *dest, char *src) {

    int r;

    if ((r = rstrncpy(&dest->s, &dest->a, dest->l, src, strlen(src))) >= 0) {
        dest->l += r;
        return 1;
    }
    return 0;
}

int StrA_copyse(StrA *dest, char *src,
                       size_t (*escape)(char *, const char *, size_t)) {

    int r;
    if ((r = rstrncpye(&dest->s, &dest->a, 0, src, strlen(src), escape)) >= 0) {
        dest->l = r;
        return 1;
    }
    return 0;
}

int StrA_catse(StrA *dest, char *src,
                       size_t (*escape)(char *, const char *, size_t)) {

    int r;
    if ((r = rstrncpye(&dest->s, &dest->a, dest->l, src, strlen(src), escape)) >= 0) {
        dest->l = r;
        return 1;
    }
    return 0;
}

int StrA_ncopys(StrA *dest, char *src, size_t ssiz) {

    int r;

    if ((r = rstrncpy(&dest->s, &dest->a, 0, src, ssiz)) >= 0) {
        dest->l = r;
        return 1;
    }
    return 0;
}

int StrA_ncats(StrA *dest, char *src, size_t ssiz) {

    int r;

    if ((r = rstrncpy(&dest->s, &dest->a, dest->l, src, ssiz)) >= 0) {
        dest->l += r;
        return 1;
    }
    return 0;
}

int StrA_ncopyse(StrA *dest, char *src, size_t ssiz,
                       size_t (*escape)(char *, const char *, size_t)) {

    int r;
    if ((r = rstrncpye(&dest->s, &dest->a, 0, src, ssiz, escape)) >= 0) {
        dest->l = r;
        return 1;
    }
    return 0;
}

int StrA_ncatse(StrA *dest, char *src, size_t ssiz,
                       size_t (*escape)(char *, const char *, size_t)) {

    int r;
    if ((r = rstrncpye(&dest->s, &dest->a, dest->l, src, ssiz, escape)) >= 0) {
        dest->l = r;
        return 1;
    }
    return 0;
}

int StrA_vprintf(StrA *str, const char *format, va_list args) {

    int r;

    if ((r = vrsprintf(&str->s, &str->a, 0, format, args)) >= 0) {
        str->l = r;
        return 1;
    }
    return 0;
}

int StrA_printf(StrA *str, const char *format, ...) {

    int r;
    va_list ap;

    va_start(ap, format);
    r = StrA_vprintf(str, format, ap);
    va_end(ap);
    return r;
}

int StrA_vcatprintf(StrA *str, const char *format, va_list args) {

    int r;

    if ((r = vrsprintf(&str->s, &str->a, str->l, format, args)) >= 0) {
        str->l += r;
        return 1;
    }
    return 0;
}

int StrA_catprintf(StrA *str, const char *format, ...) {

    int r;
    va_list ap;

    va_start(ap, format);
    r = StrA_vprintf(str, format, ap);
    va_end(ap);
    return r;
}

int StrA_truncate(StrA *str, size_t size) {

    if (str->l >= size) {
        str->l = size;
        str->s[size] = '\0';
        return 1;
    }
    return 0;
}

int StrA_cmp(StrA *str1, StrA *str2, size_t len) {

    if (len) {
        return strncmp(str1->s, str2->s, len);
    }
    return strcmp(str1->s, str2->s);
}

int StrA_casecmp(StrA *str1, StrA *str2, size_t len) {

    if (len) {
        return strncasecmp(str1->s, str2->s, len);
    }
    return strcasecmp(str1->s, str2->s);
}

char *StrA_index(StrA *str, char c) {

    size_t i;

    for (i = 0; i < str->l; i++) {
        if (str->s[i] == c) {
            return str->s + i;
        }
    }
    return NULL;
}

char *StrA_rindex(StrA *str, char c) {

    size_t i;
    for (i = str->l; i >= 0; i++) {
        if (str->s[i] == c) {
            return str->s + i;
        }
    }
}
