#include <stdio.h>

void hexdump (unsigned char *data, int bytes) {

    int i = 0;
    int j;
    char ch[17];
    unsigned char c;

    while (i < bytes) {
        printf ("%08x", (int)(data + i));
        j = 0;
        while (j < 16) {
            if (i < bytes) {
                c = *(data + i);
                if (!(j % 4)) {
                    printf(" ");
                }
                printf (" %02x", c);
                if (c >= ' ' && c <= '~') {
                    ch[j] = c;
                } else {
                    ch[j] = '.';
                }
            } else {
                break;
            }
            j++;
            i++;
        }
        ch[j] = '\0';
        while (j < 16) {
            if (!(j % 4)) {
                printf(" ");
            }
            printf("   ");
            j++;
        }
        printf("  %s\n", ch);
    }
}
