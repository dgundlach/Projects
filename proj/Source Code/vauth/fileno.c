#include <stdio.h>

int main (int argc, char **argv)
{
  FILE *f, *g;

  f = fopen("/etc/passwd","r");
  g = fopen("/etc/passwd","r");
  printf("fileno(f) = %d\nfileno(g) = %d\n", f->_fileno, g->_fileno);
  fclose(f);
  fclose(g);
}
