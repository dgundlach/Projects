#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <netinet/in.h>

in_addr_t get_ip_addr (char *ip, char **endptr) {
    
    in_addr_t nip;
    int i;
    long int n;
    char *a;
    char *e;
    
    errno = 0;
    nip = 0;
    a = ip;
    for (i = 0; i < 4; i++) {
        n = 0;
        for (e = a; (*e >= '0' && *e <= '9'); e++) {
            n = n * 10 + (*e - '0');
            if (n > 255) {
                if (endptr) {
                    *endptr = e;
                }
                errno = EINVAL;
                return 0;
            }
        }
        if (e == a || (i != 3 && *e != '.')) {
            if (endptr) {
                *endptr = e;
            }
            errno = EINVAL;
            return 0;
        }
        nip = (nip << 8) + n;
        a = e + 1;
    }
    if (endptr) {
        *endptr = e;
    }    
    return nip;
}    

int main (int argc, char **argv) {
    
    char *e;
    int i, j, c;
    in_addr_t ip;
    
    for (i = 1; i < argc; i++) {
        ip = get_ip_addr(argv[i], &e);
        if (errno != EINVAL) {
            printf("%s - Valid\n", argv[i]);
        } else {
            printf("%s - Invalid\n", argv[i]);
        }
        c = e - argv[i];
        for (j = 0; j < c; j++) printf(" ");
        printf("^\n\n");
    }
    exit(0);
}
