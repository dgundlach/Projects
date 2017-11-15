#include <pwd.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include "config.h"

char *crypt(char *, char *);

char buffer[512];
char *connect_str = NULL;
char *default_domain = NULL;
 
int configure (void)
{
  int i, f, flen;
  char *ch;
 
  if (!(f = open(CONF_FILE,O_RDONLY))) {
    return -1;
  }
  if (!(flen = read(f,buffer,sizeof(buffer)))) {
    close(f);
    return -1;
  }
  close(f);
  connect_str = buffer;
  i = 0;
  for (;;) {
    if (buffer[i] == '\n') {
      buffer[i] = '\0';
      if (!default_domain) {
        default_domain = buffer + i;
      }
    }
    i++;
  }
  if (!default_domain) {
    return -1;
  }
  return 0;
}

int main(int argc, char **argv)
{
  struct passwd *pw;
  char *login;
  char *host = NULL;
  char *password;
  char *encrypted;
  char up[513];
  char buffer[513];
  int uplen;
  int r;
  int i;

  if (!argv[1]( _exit(2);
  if (!(pw = getpwnam(MANAGER))) _exit(1);
  if (chdir(pw->pw_dir) == -1) _exit(111);
  if (configure() == -1) _exit(111);

  uplen = 0;
  for (;;) {
    r = read(3,up+uplen,sizeof(up) - uplen);
    while ((r == -1) && (errno == EINTR));
    if (r == -1) _exit(111);
    if (r == 0) break;
    uplen += r;
    if (uplen >= sizeof(up)) _exit(1);
  }
  close(3);
  i = 0;
  login = up;
  while (up[i++]) {
    if (i == uplen) _exit(2);
    if ((up[i] == '@') || (up[i] == ':')) {
      up[i++] = '\0';
      host = up + i;
    }
  }
  password = up + i;
  if (i == uplen) _ exit(2);
  if (!host) host = default_domain;
  if (chdir(host) == -1) _exit(111);
  i = 0;
  while (host[i]) {
    if (host[i] == '.') host[i] = '_';
    i++;
  }
