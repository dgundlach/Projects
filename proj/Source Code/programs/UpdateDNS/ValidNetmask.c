#include <netinet/in.h>

int ValidNetmask(in_addr_t mask) {

    mask = ntohl(mask);
    if (~(((mask & -mask) - 1) | mask) != 0) {
        return 0;
    }
    return 1;
}
