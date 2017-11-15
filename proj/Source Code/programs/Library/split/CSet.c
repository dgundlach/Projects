#include <stdio.h>
#include <stdlib.h>
#include "CSet.h"

#define BIT_MASK		0x07

static unsigned char _set_masks[] = 
                {0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80};
static unsigned char _unset_masks[] = 
                {0xfe, 0xfd, 0xfb, 0xf7, 0xef, 0xdf, 0xbf, 0x7f};

void ClearCSet(CSet *set) {

    int i;

    for(i = 0; i < BYTE_COUNT; i++) {
        set->bitfields[i] = 0;
    }
}

int CharInCSet(CSet *set, unsigned char ch) {

    return set->bitfields[ch >> 3] & _set_masks[ch & BIT_MASK];
}

void AddToCSet(CSet *set, unsigned char ch) {

    set->bitfields[ch >> 3] |= _set_masks[ch & BIT_MASK];
}

void AddRangeToCSet(CSet *set, unsigned char min, unsigned char max) {

    unsigned char byte;
    unsigned char bit;
    unsigned char i;

    if (min > max) {
        i = min;
        min = max;
        max = i;
    }
    byte = min >> 3;
    bit = min & BIT_MASK;
    for (i = min; i <= max; i++) {
        set->bitfields[byte] |= _set_masks[bit++];
        if (bit > BIT_MASK) {
            byte++;
            bit = 0;
        }
    }
}

void RemoveFromCSet(CSet *set, unsigned char ch) {

    set->bitfields[ch >> 3] &= _unset_masks[ch & BIT_MASK];
}

void RemoveRangeFromCSet(CSet *set, unsigned char min, unsigned char max) {

    unsigned char byte;
    unsigned char bit;
    unsigned char i;

    if (min > max) {
        i = min;
        min = max;
        max = i;
    }
    byte = min >> 3;
    bit = min & BIT_MASK;
    for (i = min; i <= max; i++) {
        set->bitfields[byte] &= _unset_masks[bit++];
        if (bit > BIT_MASK) {
            byte++;
            bit = 0;
        }
    }
}
