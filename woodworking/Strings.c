#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#define STRINGS_DOT_C 1
#include "Strings.h"

int strtolower(char *str) {

  char *s;

  if (!str) {
    return(1);
  }
  s = str;
  while (*s) {
    *s = tolower(*s);
    s++;
  }
  return 0;
}

int strtoupper(char *str) {

  char *s;

  if (!str) {
    return(1);
  }
  s = str;
  while (*s) {
    *s = toupper(*s);
    s++;
  }
  return 0;
}

char *astrtolower(char *str) {

  char *new;
  char *s, *n;

  if (!str) {
    return NULL;
  }
  s = str;
  while (*s++);
  if ((new = malloc(s - str + 1)) == NULL) {
    return NULL;
  }
  s = str;
  n = new;
  while (*s) {
    *n++ = tolower(*s++);
  }
  *n = '\0';
  return new;
}

char *astrtoupper(char *str) {

  char *new;
  char *s, *n;

  if (!str) {
    return NULL;
  }
  s = str;
  while (*s++);
  if ((new = malloc(s - str + 1)) == NULL) {
    return NULL;
  }
  s = str;
  n = new;
  while (*s) {
    *n++ = toupper(*s++);
  }
  *n = '\0';
  return new;
}

char *hexDigits = "0123456789ABCDEF";

char *strtohex(char *str) {

  char *hex;
  char *h, *s;

  if (!str) {
    return NULL;
  }
  s = str;
  while (*s++);
  if ((hex = malloc(2 * (s - str) + 1)) == NULL) {
    return NULL;
  }
  s = str;
  h = hex;
  while (*s) {
    *h++ = hexDigits[*s >> 4];
    *h++ = hexDigits[*s++ & 0x0f];
  }
  return hex;
}
