#include <stdio.h>
#include "funcs.h"
#include <stdlib.h>

int main (int argc, char **argv)
{
  char *line = "This:is:a:test:of:split";
  char *word, *newline;

  newline = malloc(256);
  strcpy(newline,line);
  while (newline) {
    word = Split(&newline, ":");
    printf("%s\t%s\n",word,newline);
    fflush(stdout);
  }
}
