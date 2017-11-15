#include <stdio.h>

void HexDump (void *data, int bytes, int offset) {

    int i = 0;
    int j;
    char ch[17];
    unsigned char c;
    unsigned int addr;

    if (offset == -1) {
        addr = (unsigned int)data;
    } else {
        addr = offset;
    }
    while (i < bytes) {
        printf ("%08x", addr + i);
        j = 0;
        while (j < 16) {
            if (i < bytes) {
                c = *((char *)data + i);
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
