#include <stdio.h>

int main(int argc, char **argv) {

    int i, p = 2, l2 = 0;

    for (i=0; i<256; i++) {
        if (i == p) {
            l2 += 1;
            p <<= 1;
        }
        if (!(i % 16)) {
            if (i) {
                printf("\n");
            }
            printf("               ");
        }
        printf(" %i", l2);
        if (i == 255) {
            printf("\n");
        } else {
            printf(",");
        }
    }
    exit(0);
}
