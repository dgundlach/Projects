#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <utime.h>

#define NULL 0

int touch(char *filename) {

  int f;

  f = open(filename,O_WRONLY|O_NONBLOCK|O_CREAT|O_NOCTTY,0666);
  if (!f) {
    return -1;
  }
  close(f);
  return utime(filename,NULL);
}
