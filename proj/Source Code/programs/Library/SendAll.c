#include <sys/types.h>
#include <sys/socket.h>

int SendAll(int s, char *buf, int *len, int flags) {

    int total = 0;
    int bytesleft = *len;
    int n;
    
    while (total < *len) {
        n = send(s, buf + total, bytesleft, flags);
        if (n == -1) {
            break;
        }
        total += n;
        bytesleft -= n;
    }
    *len = total;
    return (n == -1) ? -1 : 0;
}
