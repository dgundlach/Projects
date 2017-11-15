#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include "get_subnet.h"

#define IP_PATTERN                 "%u.%u.%u.%u%c%n"

struct subnet *get_subnet(char *buf) {

    in_addr_t i, j, k;
    static in_addr_t masks[33];
    static int masks_set = 0;
    static struct subnet net;
    in_addr_t a0, a1, a2, a3;
    char c;
    int p, e;
    int mi;
    char *x;

    if (!masks_set) {
        k = i = 0x80000000U;
        masks[0] = 0;
        for (j = 1; j < 33; j++) {
            masks[j] = htonl(i);
            k = k >> 1;
            i = i + k;
        }
        masks_set = 1;
    }
    e = sscanf(buf, IP_PATTERN, &a0, &a1, &a2, &a3, &c, &p);
    if (e > 3 && a0 < 256 && a1 < 256 && a2 < 256 && a3 < 256) { 
        net.ip_address = htonl((((((a0 << 8) + a1) << 8) + a2) << 8) + a3);
        if (e == 5) {
            if (c == '/') {
                mi = strtoul(buf + p, &x, 10);
                if (*x == '.') {
                    e = sscanf(buf + p, IP_PATTERN, &a0, &a1, &a2, &a3, &c, &p);
                    if (e == 4 && a0 < 256 && a1 < 256 && a2 < 256 && a3 < 256) {
                        net.netmask = (((((a0 << 8) + a1) << 8) + a2) << 8) + a3;
                        if (~(((net.netmask & -(net.netmask)) - 1) | net.netmask) != 0) {
                            errno = EINVAL;
                            return NULL;
                        } else {
                            net.netmask = htonl(net.netmask);
                        }
                    } else {
                        errno = EINVAL;
                        return NULL;
                    }
                } else {
                    if (x != (buf + p) && !*x && mi < 33) {
                        net.netmask = masks[mi];
                    } else {
                        errno = EINVAL;
                        return NULL;
                    }
                }
            } else {
                errno = EINVAL;
                return NULL;
            }
        } else {
            net.netmask = masks[24];
        }
    } else {
        errno = EINVAL;
        return NULL;
    }
    net.network_address = net.ip_address & net.netmask;
    net.broadcast_address = net.network_address | ~(net.netmask);
    return &net;
}
