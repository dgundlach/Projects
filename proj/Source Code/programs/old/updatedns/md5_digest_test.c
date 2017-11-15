#include <stdio.h>
#include <string.h>
#include "md5.h"

char *md5_digest(char *str) {

    char hex[] = "0123456789abcdef";
    static char md5str[33];
    MD5_CTX ctx;
    unsigned char digest[16];
    int i;

    md5str[0] = '\0';
    MD5Init(&ctx);
    MD5Update(&ctx, str, strlen(str));
    MD5Final(digest, &ctx);
    for (i = 0; i < 16; i++) {
	md5str[2 * i] = hex[digest[i] / 16];
	md5str[(2 * i) + 1] = hex[digest[i] % 16];
    }
    md5str[32] = '\0';
    return md5str;
}

char *md5_digest2(char *str) {

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

int main(int argc, char **argv) {

    if (argc == 2) {
	printf("%s\n", md5_digest2(argv[1]));
    }
    exit(0);
}
