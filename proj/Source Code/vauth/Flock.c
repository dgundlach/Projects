#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

int SetLock (char *lock)
{
  int fd;

  if ((fd = open(lock,O_RDONLY)) != -1) {
    close(fd);
    return -1;
  }
  if ((fd = open(lock,O_WRONLY | O_CREAT)) == -1)
    return -1;
  close(fd);
  return 0;
}

int DropLock (char *lock)
{
  return unlink(lock);
}
