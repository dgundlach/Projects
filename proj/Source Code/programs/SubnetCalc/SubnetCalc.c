#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "GetSubnet.h"
#include "BitCount.h"
#include "IntLog2.h"

#define H1 "     Network          Broadcast           Netmask               Cisco\n"
#define H2 "     Address           Address      Bits      Mask             Wildcard\n"
#define H3 "---------------------------------------------------------------------------\n"
#define XX " xxx.xxx.xxx.xxx   xxx.xxx.xxx.xxx   xx  xxx.xxx.xxx.xxx   xxx.xxx.xxx.xxx"
#define DETAIL  " %15s   %15s   %2i  %15s   %15s\n"

void usage(char *prog) {

    char *bn;

    if ((bn = strrchr(prog, '/'))) {
        bn++;
    } else {
        bn = prog;
    }
    printf("Usage: %s <ip[/netmask]> <number of hosts>\n", bn);
}

int main (int argc, char **argv) {

    struct subnet *s;
    unsigned long int nhosts;
    int bits, nbits;
    int networks;
    int inc;
    int n;
    char *e;
    struct in_addr addr, netmask, broadcast, cisco;
    char addr_buff[INET6_ADDRSTRLEN];
    char netmask_buff[INET6_ADDRSTRLEN];
    char broadcast_buff[INET6_ADDRSTRLEN];
    char cisco_buff[INET6_ADDRSTRLEN];

    if (argc != 3) {
	    usage(argv[0]);
        exit(1);
    }
    if (!(s = GetSubnet(argv[1]))) {
        printf("%s - Invalid argument.\n", argv[1]);
        usage(argv[0]);
        exit(1);
    }
    nhosts = strtoul(argv[2], &e, 10);
    if (*e || !nhosts) {
        printf("%s - Invalid argument.\n", argv[2]);
        usage(argv[0]);
        exit(1);
    }
    switch (nhosts) {
    case 1:
        bits = 0;
        break;
    case 2:
    case 3:
        bits = 2;
        break;
    default:
        bits = IntLog2(nhosts - 1) + 1;
    }
    nbits = 32 - BitCount(s->netmask.s_addr);
    if (nbits < bits) {
        printf("Not enough room in your network for %li hosts.\n", nhosts);
        exit(1);
    }
    networks = 1 << ((32 - BitCount(s->netmask.s_addr)) - bits);
    netmask.s_addr = htonl(037777777777 << bits);
    cisco.s_addr = ~(netmask.s_addr);
    inc = 1 << bits;
    addr = s->network;
    inet_ntop(AF_INET, &netmask, netmask_buff, sizeof(netmask_buff));
    inet_ntop(AF_INET, &cisco, cisco_buff, sizeof(cisco_buff));
    bits = 32 - bits;
    printf("\n" H1 H2 H3);
    for (n = 0; n < networks; n++) {
        broadcast.s_addr = addr.s_addr | ~(netmask.s_addr);
        inet_ntop(AF_INET, &addr, addr_buff, sizeof(addr_buff));
        inet_ntop(AF_INET, &broadcast, broadcast_buff, sizeof(broadcast_buff));
        printf(DETAIL, addr_buff, broadcast_buff, bits, netmask_buff, cisco_buff);
        addr.s_addr = htonl(ntohl(addr.s_addr) + inc);
    }
    printf(H3 "\n");
    exit(0);
}
