#include <sys/types.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/inotify.h>
#include <errno.h>

#define LC "abcdefghijklmnopqrstuvwxyz"
#define MTAB "/etc/mtab"
#define DEVDIR "/dev"

void touchsddevices(void) {

    DIR *dev;
    struct dirent *entry;
    int len;
    int fd;
    char path[256];

//
// This loop opens each sd device for reading, then closes it.  Apparently
// this is enough to trigger the kernel to call udev on the device and scan
// for any partitions.
//

    if (dev = opendir("/dev")) {
        while (entry = readdir(dev)) {
            if (((len = strlen(entry->d_name)) <= 4)
                    && (entry->d_name[0] == 's')
                    && (entry->d_name[1] == 'd')
                    && (strspn(entry->d_name, LC) == len)) {
                sprintf(path, "/dev/%s", entry->d_name);
                fd = open(path,  O_RDONLY|O_NOCTTY|O_NONBLOCK);
                close(fd);
            }
        }
        closedir(dev);
    }
}

#define EVENT_SIZE (sizeof(struct inotify_event))
#define BUF_LEN (16 * EVENT_SIZE)

int main(int argc, char **argv) {

    int fd;
    int watch;
    char buf[BUF_LEN];
    int len, i, c;
    struct inotify_event *event;

    fd = inotify_init();
    if (fd < 0) {
        exit(-1);
    }
//    watch = inotify_add_watch(fd, MTAB, IN_CLOSE_WRITE);
    watch = inotify_add_watch(fd, MTAB, IN_ALL_EVENTS);
    if (watch < 0) {
        exit(-1);
    }
//    watch = inotify_add_watch(fd, DEVDIR, IN_CREATE | IN_DELETE 
//                                                    | IN_DONT_FOLLOW);
    watch = inotify_add_watch(fd, DEVDIR, IN_CREATE | IN_ALL_EVENTS);
    if (watch < 0) {
        exit(-1);
    }
    while (1) {
        i = 0;
        len = read(fd, buf, BUF_LEN);
        if (len < 0) {
            if (!(errno == EINTR)) {
                perror("Inotify read error.\n");
            }
        }
        while (i < len) {
             event = (struct inotify_event *) &buf[i];
             if (strcmp(event->name, "ptmx"))
                 printf("%s %i\n", event->name, event->mask);
             i += EVENT_SIZE + event->len;
        }
    }
}
