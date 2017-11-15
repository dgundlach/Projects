#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <getopt.h>
#include "IntLog2.h"

void usage(char *prog) {

    char *bn;

    if ((bn = strrchr(prog, '/'))) {
        bn++;
    } else {
        bn = prog;
    }
    printf("Usage: %s [-m|b] <number of hosts>\n", bn);
}

int main(int argc, char **argv) {

    unsigned long int nhosts;
    int bits;
    char *e;
    struct in_addr netmask;
    char netmask_buff[INET6_ADDRSTRLEN];
    int outtype = 0;

    while (1) {
        int c = getopt(argc, argv, "mb");
        if (c == -1) {
            break;
        }
        switch(c) {
        case 'b':
            if (outtype) {
                printf("Switches -b and -m are exclusive.\n");
                usage(argv[0]);
                exit(1);
            }
            outtype = 1;
            break;
        case 'm':
	        if (outtype) {
		        printf("Switches -b and -m are exclusive.\n");
		        usage(argv[0]);
		        exit(1);
	        }
	        outtype = 2;
	        break;
	    default:
	        usage(argv[0]);
	        exit(1);
	    }
    }
    if (!argv[optind]) {
        usage(argv[0]);
        exit(1);
    }
    nhosts = strtoul(argv[optind], &e, 10);
    if (*e | !nhosts) {
        printf("%s - Invalid argument.\n", argv[optind]);
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
    netmask.s_addr = htonl(0xffffffffu << bits);
    bits = 32 - bits;
    inet_ntop(AF_INET, &netmask, netmask_buff, sizeof(netmask_buff));
    switch(outtype) {
    case 0:
        printf("Netmask = %s\nBits    = %i\n", netmask_buff, bits);
        break;
    case 1:
        printf("%i\n", bits);
        break;
    case 2:
        printf("%s\n", netmask_buff);
    }
    exit(0);
}
