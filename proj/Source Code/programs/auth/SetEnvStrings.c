#include <stdlib.h>

extern char **envstrings;

void SetEnvString(char *key, char *value) {

  static int ix = 0;
  static int maxix = 0;
  char *tmp;

  if (ix == maxix) {
    maxix += 30;
    tmp = realloc(envstrings,1 + maxix * (sizeof(char *)));
    envstrings = (char **)tmp;
  }
  envstrings[ix] = malloc(strlen(key) + strlen(value) + 2);
  sprintf(envstrings[ix],"%s=%s",key,value);
  ix++;
  envstrings[ix] = NULL;
}
