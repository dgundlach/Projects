#include <stdlib.h>

char *Stringify (char *str) {

  char *newstr;
  char *optr, *nptr;

  newstr = malloc((2 * strlen(str)) + 2);
  optr = str;
  nptr = newstr;
  *nptr++ = '\'';
  while (*optr) {
    if (*optr == '\'') *nptr++ = '\\';
    *nptr++ = *optr++;
  }
  *nptr++ = '\'';
  *nptr++ = '\0';
  newstr = realloc(newstr, nptr - newstr);
  return newstr;
}
