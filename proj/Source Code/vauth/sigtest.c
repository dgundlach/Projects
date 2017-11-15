#include <stdio.h>
#include <signal.h>

void catch_sigint (int signum)
{
  signal(SIGINT, catch_sigint);
  printf("Stop that!\n");
  fflush(stdout);
}

int main (int argc, char **argv)
{
  int i;
  char buffer[256];

  signal(SIGINT, catch_sigint);
  for (i=0; i<10; i++) {
    fgets(buffer,256,stdin);
  }
}
