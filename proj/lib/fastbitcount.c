#include <stdint.h>

static char bits_in_char[] = {
		0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4,
		1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
		1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
		2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
		1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
		2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
		2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
		3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
		1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
		2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
		2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
		3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
		2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
		3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
		3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
		4, 5, 5, 6, 5, 6, 6, 7, 5, 6, 6, 7, 6, 7, 7, 8
};

#define MASK		0xffu

int bitcount8(uint8_t n) {

	register char bc;

	bc  = bits_in_char[n & MASK];
	return (int)bc;
}

int bitcount16(uint16_t n) {

	register char bc;

	bc  = bits_in_char[n & MASK];
	n >>= 8;
	bc += bits_in_char[n & MASK];
	return (int)bc;
}

int bitcount32(uint32_t n) {

	register char bc;

	bc  = bits_in_char[n & MASK];
	n >>= 8;
	bc += bits_in_char[n & MASK];
	n >>= 8;
	bc += bits_in_char[n & MASK];
	n >>= 8;
	bc += bits_in_char[n & MASK];
	return (int)bc;
}

int bitcount64(uint64_t n) {

	register char bc;

	bc  = bits_in_char[n & MASK];
	n >>= 8;
	bc += bits_in_char[n & MASK];
	n >>= 8;
	bc += bits_in_char[n & MASK];
	n >>= 8;
	bc += bits_in_char[n & MASK];
	n >>= 8;
	bc += bits_in_char[n & MASK];
	n >>= 8;
	bc += bits_in_char[n & MASK];
	n >>= 8;
	bc += bits_in_char[n & MASK];
	n >>= 8;
	bc += bits_in_char[n & MASK];
	return (int)bc;
}
