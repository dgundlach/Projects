#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include "GetSubnet.h"

#define IP_PATTERN                 "%u.%u.%u.%u%c%n"
#define DEFAULT_NETMASK            037777777400u

struct subnet *GetSubnet(char *buf) {

    static struct subnet net;
    in_addr_t a0, a1, a2, a3;
    char c;
    int p, e;
    int mi;
    char *x;

    e = sscanf(buf, IP_PATTERN, &a0, &a1, &a2, &a3, &c, &p);
    if (e > 3 && a0 < 256 && a1 < 256 && a2 < 256 && a3 < 256) { 
        net.ip_addr.s_addr = htonl((((((a0 << 8) + a1) << 8) + a2) << 8) + a3);
        if (e == 5) {
            if (c == '/') {
                mi = strtoul(buf + p, &x, 10);
                if (*x == '.') {
                    e = sscanf(buf + p, IP_PATTERN, &a0, &a1, &a2, &a3, &c, &p);
                    if (e == 4 && a0 < 256 && a1 < 256 && a2 < 256 && a3 < 256) {
                        net.netmask.s_addr = (((((a0 << 8) + a1) << 8) + a2) << 8) + a3;
                        if (~(((net.netmask.s_addr & -(net.netmask.s_addr)) - 1) | net.netmask.s_addr) != 0) {
                            errno = EINVAL;
                            return NULL;
                        } else {
                            net.netmask.s_addr = htonl(net.netmask.s_addr);
                        }
                    } else {
                        errno = EINVAL;
                        return NULL;
                    }
                } else {
                    if (x != (buf + p) && !*x && mi < 33) {
                        net.netmask.s_addr = htonl(037777777777u << (32 - mi));
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
            net.netmask.s_addr = htonl(DEFAULT_NETMASK);
        }
    } else {
        errno = EINVAL;
        return NULL;
    }
    net.network.s_addr = net.ip_addr.s_addr & net.netmask.s_addr;
    net.broadcast.s_addr = net.network.s_addr | ~(net.netmask.s_addr);
    return &net;
}
