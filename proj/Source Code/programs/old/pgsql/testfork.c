#include <stdio.h>
#include <unistd.h>





int main (int argc, char **argv)
{
  FILE *f;
  int pid;
  char buffer[1000];

  f = fopen("/etc/passwd","r");
  pid = fork();
  if (!pid) {
    printf("In child\n");
    fgets(buffer,1000,f);
    while (!feof(f)) {
      printf("child: %s",buffer);
      sleep(2);
      fgets(buffer,1000,f);
    }
  } else {
    printf("In child\n");
    fgets(buffer,1000,f);
    while (!feof(f)) {
      printf("parent: %s",buffer);
      sleep(1);
      fgets(buffer,1000,f);
    }
  }
  fclose(f);
}
