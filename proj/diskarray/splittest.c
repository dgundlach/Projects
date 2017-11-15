#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

#include "split2.h"
#include "readtextline.h"

#define MAJOR 0
#define MINOR 1
#define DEVICE 3

int main (int argc, char **argv) {

	char *buf = NULL;
	char *parts[64];
	int count;
	int i = 0;
	dev_t device;

	read_text_line(&buf, "/proc/partitions");
	read_text_line(&buf, "/proc/partitions");
	while (read_text_line(&buf, "/proc/partitions") == READTEXTLINE_SUCCESS) {
		count = split(buf, parts, 5, NULL);
		device = atoi(parts[MAJOR]) * 256 + atoi(parts[MINOR]);
		printf("%s\t 0x%04x\n", parts[DEVICE], device);
	}
}
