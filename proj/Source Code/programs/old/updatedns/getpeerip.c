#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

static char _addr_buff[INET6_ADDRSTRLEN];

char *getpeerip(int fd) {

    struct sockaddr name;
    int namelen = sizeof(name);
    struct in6_addr addr6;
    struct in_addr addr;

    if (getpeername(fd, (struct sockaddr *)&name, &namelen) == -1) {
	return NULL;
    } else {
        if (name.sa_family == AF_INET) {
	    addr = ((struct sockaddr_in *)&name)->sin_addr;
            (void) inet_ntop(AF_INET, &addr, _addr_buff, sizeof (_addr_buff));
        } else if (name.sa_family == AF_INET6) {
	    addr6 = ((struct sockaddr_in6 *)&name)->sin6_addr;
            (void) inet_ntop(AF_INET6, &addr6, _addr_buff, sizeof (_addr_buff));
        }
        return _addr_buff;
    }
}
