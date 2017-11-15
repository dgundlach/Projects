#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>

time_t mod_time(char *filename) {

  struct stat st;

  if (!stat(filename,&st)) {
    return st.st_mtime;
  } else {
    return (time_t) 0;
  }
}

time_t creat_time(char *filename) {

  struct stat st;

  if (!stat(filename,&st)) {
    return st.st_ctime;
  } else {
    return (time_t) 0;
  }
}
