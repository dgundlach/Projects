
#include <string.h>

char *Split (char **str, char *delim)
{
  char *ch;
  char *token;

  if (!str)
    return "\0";
  token = *str;
  if ((ch = strpbrk(token, delim))) {
    *ch = '\0';
    *str = ch + 1;
  } else {
    if ((ch = strpbrk(token, "\r\n")))
      *ch = '\0';
    *str = NULL;
  }
  return token;
}
