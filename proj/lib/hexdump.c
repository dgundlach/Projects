#include <stdio.h>

void hexdump (void *data, int bytes) {

	long i = 0;
	long j;
	char ch[17];
	unsigned char c;
	unsigned char *d = data;

	while (i < bytes) {
		printf ("%08x", (long)(d + i));
		j = 0;
		while (j < 16) {
			if (i < bytes) {
				c = *(d + i);
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
