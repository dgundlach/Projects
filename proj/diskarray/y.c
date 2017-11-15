#include <stdio.h>
#include "fgettext.h"

int main (int argc, char **argv) {

	char *buf;
	int size;
	int len;

	while ((len = fgettext(&buf, &size, "fgettext.c")) > 0) {
		printf("size = %d, len = %d\n", size, len);
		printf(buf);
	}
}
	
