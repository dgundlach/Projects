#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <strings.h>

int main(int argc, char **argv) {

    char buf[4194304];
    int fd, i;

    fd = open("z1", O_WRONLY|O_CREAT, 0644);
    bzero(buf, 4194304);
    for (i=0; i<256; i++)
        write(fd, buf, 4194304);
    close(fd);
}
