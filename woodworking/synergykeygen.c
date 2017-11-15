#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>

#define HEX "0123456789ABCDEF"

char *strtohex(unsigned char *str) {

  char *hex;
  char *h, *s;
  char *digits = HEX;

  hex = malloc(2 * strlen(str) + 1);
  s = str;
  h = hex;
  while (*s) {
    *h++ = digits[*s >> 4];
    *h++ = digits[*s++ & 0x0f];
  }
  return hex;
}

int main(int argc, char **argv) {

  unsigned char *name;
  unsigned char *email;
  unsigned char *company;
  unsigned char license[256];

  name = "Freddy Kreuger";
  email = "fkreuger@elmstreet.com";
  company = "Claws, Inc.";

  if (argc > 1) {
    name = argv[1];
    email = argv[2];
    company = argv[3];
  }
  snprintf(license, 256, "{v1;pro;%s;10;%s;%s;0;0}", name, email, company);

  printf("%s\n", strtohex(license));
  exit(0);
}
