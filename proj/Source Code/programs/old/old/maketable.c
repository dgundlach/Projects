#include <stdio.h>
#include "BitCount.h"

int main(int argc, char **argv) {

    int i;

    printf("static char bits_in_char[] = {");
    for (i = 0; i < 256; i++) {
	if (!(i % 16)) {
	    printf("\n\t\t");
	}
	printf("%i", BitCount(i));
        if (i != 255) {
	    printf(",");
	    if ((i + 1) % 16) {
		printf(" ");
	    }
	}
    }
    printf("\n};\n");
    return(0);
}
