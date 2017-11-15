#include <string.h>
#include "md5.h"

char *md5_digest(char *str) {

    char hex[] = "0123456789abcdef";
    char *md5str;
    MD5_CTX ctx;
    unsigned char digest[17];
    char *m, *d;
    unsigned char c;

    md5str = malloc(33);
    MD5Init(&ctx);
    MD5Update(&ctx, str, strlen(str));
    MD5Final(digest, &ctx);
    digest[16] = '\0';
    m = md5str;
    d = digest;
    while (c = *d++) {
        *m++ = hex[c / 16];
        *m++ = hex[c % 16];
    }
    *m = '\0';
    return md5str;
}
