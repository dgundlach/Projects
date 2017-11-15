#include <string.h>
#include "md5.h"

char *MD5Digest(char *str) {

    char hex[] = "0123456789abcdef";
    char *md5str;
    MD5_CTX ctx;
    unsigned char digest[16];
    register unsigned char *m, *d;

    md5str = malloc(33);
    MD5Init(&ctx);
    MD5Update(&ctx, str, strlen(str));
    MD5Final(digest, &ctx);
    m = md5str;
    d = digest;
    *m++ = hex[*d >> 4];
    *m++ = hex[*d++ & 0x0fu];
    *m++ = hex[*d >> 4];
    *m++ = hex[*d++ & 0x0fu];
    *m++ = hex[*d >> 4];
    *m++ = hex[*d++ & 0x0fu];
    *m++ = hex[*d >> 4];
    *m++ = hex[*d++ & 0x0fu];
    *m++ = hex[*d >> 4];
    *m++ = hex[*d++ & 0x0fu];
    *m++ = hex[*d >> 4];
    *m++ = hex[*d++ & 0x0fu];
    *m++ = hex[*d >> 4];
    *m++ = hex[*d++ & 0x0fu];
    *m++ = hex[*d >> 4];
    *m++ = hex[*d++ & 0x0fu];
    *m++ = hex[*d >> 4];
    *m++ = hex[*d++ & 0x0fu];
    *m++ = hex[*d >> 4];
    *m++ = hex[*d++ & 0x0fu];
    *m++ = hex[*d >> 4];
    *m++ = hex[*d++ & 0x0fu];
    *m++ = hex[*d >> 4];
    *m++ = hex[*d++ & 0x0fu];
    *m++ = hex[*d >> 4];
    *m++ = hex[*d++ & 0x0fu];
    *m++ = hex[*d >> 4];
    *m++ = hex[*d++ & 0x0fu];
    *m++ = hex[*d >> 4];
    *m++ = hex[*d++ & 0x0fu];
    *m++ = hex[*d >> 4];
    *m++ = hex[*d & 0x0fu];
    *m = '\0';
    return md5str;
}
