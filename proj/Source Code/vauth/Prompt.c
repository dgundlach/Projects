#include <stdio.h>
#include <string.h>
#include <stdlib.h>

char *Prompt (char *txt, int pad, char *def, int anslen)
{
  char *answer = NULL;
  char *temp = NULL;
  int p;

  p = printf("%s", txt);
  if (def) p += printf(" [%s]", def);
  while (p < pad) {
    printf(" ");
    p++;
  }
  printf(" -> ");

  answer = malloc(anslen + 1);
  fgets(answer, anslen + 1, stdin);
  p = strlen(answer) - 1;
  if (answer[p] == '\n') {
    answer[p] = '\0';
  } else {
    temp = malloc(256);
    for (;;) {
      fgets(temp, 256, stdin);
      if (temp[strlen(temp) - 1] == '\n')
        break;
    }
  } 
  if (! *answer && def) 
    strncpy(answer, def, anslen + 1);
  return answer;
}
