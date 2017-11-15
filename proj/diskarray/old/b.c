#include <stdio.h>
#include <ctype.h>

//#define DECODE_HEX(p) (isxdigit(*p)) ? ((*p >= 'a') ? *p++ - 'a' + 10: *p++ - '0') : 0
//#define DECODE_HEX(p) ((isxdigit(*p)) ? ((*p >= 'a') ? *p++ - 'a' + 10: *p++ - '0') : 0)
#define DECODE_HEX(p) ((isxdigit(*p)) ? ((*p <= '9') ? *p++ - '0' : tolower(*p++) - 'a' + 10) : 0)
int main(int argc, char **argv) {

	char hn[] = "19Da";
	char *p = hn;
	int device = 0;

	device = DECODE_HEX(p);
	printf("%d %x\n", device, device);
	device = (device * 16) + DECODE_HEX(p);
	printf("%d %x\n", device, device);
	device = (device * 16) + DECODE_HEX(p);
	printf("%d %x\n", device, device);
	device = (device * 16) + DECODE_HEX(p);
	printf("%d %x\n", device, device);
}
