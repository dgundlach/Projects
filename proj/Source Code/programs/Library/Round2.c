#include "IntLog2.h"

int Round2(unsigned int n) {

    if (!n) {
        return 0;
    }
    return 1 << (IntLog2(n - 1) + 1);
}
