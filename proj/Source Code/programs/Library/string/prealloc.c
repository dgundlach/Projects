#include <stdlib.h>

static unsigned int _intlog2(unsigned int);
static unsigned int _round2(unsigned int);

/*
 * Value of _shift determines the divisor for allocating the buffer size.
 * Value should be 5 at a minimum, which would allocate 32 byte chunks.
 * Increasing _shift by 1 doubles the allocation size.  The _pad should be
 * half of the allocation size if it is less than 256, one quarter otherwise.
 * The pad size should not be more than 256, though.
 */

static short _shift =  8;     // 2**8 = 256 characters.
static short _pad   = 64;     // One quarter of the allocation size.

char *prealloc(char *buf, size_t *request, int pad) {

    char *nb;
    size_t nr;

    pad = (pad) ? _pad : 0;

//  Add a padding to the number of characters requested, then round it up
//  to the allocation size.  The amount of memory allocated will always be
//  a multiple of the allocation size.

    nr = (((*request + pad) >> _shift) + 1) << _shift;
    if (!(nb = realloc(buf, nr))) {
        return NULL;
    }
    *request = nr;
    return nb;
}

void set_prealloc_size(unsigned short size) {

    size   = (size < 32)   ? 32                : size;
    _shift = (size > 4096) ? _intlog2(4096)    : _intlog2(size);
    _pad   = (size > 128)  ? 1 << (_shift - 2) : 1 << (_shift - 1);
    _pad   = (_pad > 256)  ? 256               : _pad;
}

unsigned int _intlog2(unsigned int n) {

    static const unsigned int b[] = {0x2, 0xC, 0xF0, 0xFF00, 0xFFFF0000};
    static const unsigned int s[] = {1, 2, 4, 8, 16};
    register unsigned int c = 0;
    int i;

    for (i = 4; i >= 0; i--) {
        if (n & b[i]) {
            n >>= s[i];
            c |= s[i];
        } 
    }
    return c;
}

unsigned int _round2(unsigned int n) {

    if (!n) {
        return 0;
    }
    return 1 << (_intlog2(n - 1) + 1);
}
