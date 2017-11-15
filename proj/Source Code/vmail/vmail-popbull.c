#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <linux/errno.h>
#include <fcntl.h>

extern int errno;
char *prog;

static char newname[128];

void usage(void) {

  fprintf(stderr,"usage: %s <bulldir> <program> <maildir>",prog);
  exit(1);
}

char *maildir_makefilename(void) {

  unsigned long pid;
  time_t now;
  char host[64];
  char *newname;
  struct stat st;
  int loop;
  int fd;

  pid = getpid();
  host[0] = '\0';
  gethostname(host,sizeof(host));
  for (loop=0;;++loop) {
    now = time(NULL);
    snprintf(newname,sizeof(newname),"new/%d.%d.%s",now,pid,host);
    if (stat(newname,&st) == -1)
      if (errno == ENOENT)
        break;
    if (loop == 2)
      exit(1);
    sleep(2);
  }
  return newname;
}

int main(int argc, char **argv) {

  char *bulldirname;
  char *programname;
  char *maildirname;
  time_t timestamp;
  int fd;
  char bulletin[256];
  char *bullname;
  DIR *bulldir;
  struct dirent *d;
  struct stat st;

  if ((prog = strrchr(argv[0],'/'))) {
    prog++;
  } else {
    prog = argv[0];
  }
  if (!(bulldirname = argv[1]))
    usage();
  if (!(programname = argv[2]))
    usage();
  if (!(maildirname = argv[3]))
    usage();
  if (chdir(maildirname) == -1) {
    umask(077);
    mkdir(maildirname,0600);
    if (chdir(maildirname) == -1) {
      exit(1);
    } else {
      mkdir("cur",0600);
      mkdir("new",0600);
      mkdir("tmp",0600);
    }
  }
  if (stat(".timestamp",&st) == -1) {
    timestamp = 0;
  } else {
    timestamp = st.st_mtime;
  }
  fd = open(".timestamp",O_CREAT|O_TRUNC,0600);
  close(fd);

  bullname = bulletin;
  bullname += sprintf(bulletin,"%s/",bulldirname);
  if ((bulldir = opendir(bulldirname))) {
    while ((d = readdir(bulldir))) {
      if (!strcmp(d->d_name,".")) continue;
      if (!strcmp(d->d_name,"..")) continue;
      strcpy(bullname,d->d_name);
      if (stat(bulletin,&st) == -1) exit(1);
      if ((st.st_mode & 0222) == 0) continue;
      if (st.st_mtime > timestamp) {
        maildir_makefilename();
        symlink(bulletin,newname);
      }
    }
    closedir(bulldir);
  }
  argv[argc - 1] = ".";
  execvp(argv[2],argv + 2);
}
