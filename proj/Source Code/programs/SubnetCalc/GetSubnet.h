#include <netinet/in.h>

struct subnet {
    struct in_addr ip_addr;
    struct in_addr network;
    struct in_addr broadcast;
    struct in_addr netmask;
};

struct subnet *GetSubnet(char *);
