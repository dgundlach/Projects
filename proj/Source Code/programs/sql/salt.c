#include <stdlib.h>
#include <time.h>

char *salt(int type) {

  static char *b64             = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                 "abcdfeghijklmnopqrstuvwxyz"
		                 "0123456789./";
  static char SodiumChloride[] = "$1$12345678$";
  char        *NaCl;

  srand(time(NULL) << 8);
  NaCl = SodiumChloride + 3;
  while (*NaCl != '$') {
    *NaCl++ = b64[(int)(64.0 * rand() / (RAND_MAX + 1.0))];
  }
  if (type == 0) {
    SodiumChloride[5] = '\0';
    return SodiumChloride + 3;
  }
  return SodiumChloride;
}
