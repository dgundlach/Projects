/*
 * $Id: rsprintf.c v1.0 2005-04-08 00:39:02 djg
 *
 * rsprintf, vrsprintf
 *
 * Print to a string according to a format.  If the string is NULL, or
 * is not large enough, (re)allocate it.
 *
 * rsprintf is called with a variable number of arguments following the 
 * format string.
 *
 * vrsprintf is called with a va_list instead of a variable number of 
 * arguments.  The application should call the va_end macro after calling
 * vrsprintf.
 *
 * Parameters:
 *
 *      **str       A pointer to the string variable.
 *      *size       A pointer to the variable containing the string size.
 *      offset      The point in the string to begin writing.
 *      *format     The format in which to print the rest of the arguments.
 *
 * Return value:
 *
 *      The number of characters printed (not including the trailing \0
 *      used to end output strings).  If the string could not be (re)alloced,
 *      -1 is returned.
 */

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include "strfuncs.h"

int vrsprintf(char **str, size_t *size, size_t offset,
                                        const char *format, va_list args) {

    char x;
    char *buff = NULL;
    int ns, off;

    if (!str || !size) {
        return -1;
    }

//  Calculate the amount of space we have left in the string.  If the value
//  is zero, there's no more room.  If the value is negative, we're going 
//  to have some undefined characters preceding the output.  This is not
//  necessarily a bad thing, but the programmer has to be careful with what
//  he is doing.

    ns = *size - offset;
    if (*str && ns > 0) {
        buff = *str;        
        off = offset;                        
    } else {            //  No space left.  Point to a temporary buffer.
        ns = 1;
        buff = &x;
        off = 0;
    }

//  Print the string, or at least attempt to.

    ns = offset + vsnprintf(buff + off, ns, format, args);
    if (ns >= *size || buff != *str) {

//      If there wasn't any room in the string, we need to allocate more room
//      and reprint the string.

        if ((buff = prealloc(*str, &ns))) {
            *size = ns;
            *str = buff;
        } else {
            return -1;
        }
        ns = vsnprintf(buff + offset, ns, format, args);
    } else {
        ns -= offset;
    }
    return ns;
}

int rsprintf(char **str, size_t *size, size_t offset, 
                                       const char *format, ...) {

    va_list ap;
    int ret;

//  This one is simple.  Just set up a call to the other one.

    va_start(ap, format);
    ret = vrsprintf(str, size, offset, format, ap);
    va_end(ap);
    return ret;
}

int rstrncpy(char **dest, size_t *dsize, size_t offset, const char *src, 
                                                        size_t ssize) {

    int i;

    if (!str_ready(dest, dsize, offset + ssize)) {
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
        if (off >= *dsize && !str_ready(dest, dsize, off + strlen(src + 1) + 1)) {
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


    if (!str_ready(dest, dsize, offset + (ssize << 1))) {
        return -1;
    }
    return escape(*dest + offset, src, ssize);
}

int rstrcpye(char **dest, size_t *dsize, size_t offset, const char *src,
                       size_t (*escape)(char *, const char *, size_t)) {

    return rstrncpye(dest, dsize, offset, src, strlen(src), escape);
}

int str_ready(char **str, size_t *oldsize, size_t newsize) {

    int ns;
    char *buff;

    if (newsize >= *oldsize) {
        ns = newsize;
        if ((buff = prealloc(*str, &ns))) {
            *oldsize = ns;
            *str = buff;
        } else {
            return 0;
        }
    }
    return 1;
}

int StrA_ready(StrA *str, size_t size) {

    return str_ready(&str->s, &str->a, str->l + size);
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

    if (str_ready(&str->s, &str->a, str->l + 1)) {
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
    if (str_ready(&str->s, &str->a, count)) {
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

int StrA_copya(StrA *dest, StrA *src) {

    int r;

    if ((r = rstrncpy(&dest->s, &dest->a, 0, src->s, src->l)) >= 0) {
        dest->l = r;
        return 1;
    }
    return 0;
}

int StrA_cata(StrA *dest, StrA *src) {

    int r;

    if ((r = rstrncpy(&dest->s, &dest->a, dest->l, src->s, src->l)) >= 0) {
        dest->l += r;
        return 1;
    }
    return 0;
}

int StrA_copyae(StrA *dest, StrA *src,
                       size_t (*escape)(char *, const char *, size_t)) {

    int r;
    if ((r = rstrncpye(&dest->s, &dest->a, 0, src->s, src->l, escape)) >= 0) {
        dest->l = r;
        return 1;
    }
    return 0;
}

int StrA_catae(StrA *dest, StrA *src,
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
