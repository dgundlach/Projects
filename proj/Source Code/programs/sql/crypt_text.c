#define _XOPEN_SOURCE
#include <stdio.h>
#include <postgres.h>
#include <fmgr.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

PG_FUNCTION_INFO_V1(crypt_text);

Datum crypt_text(PG_FUNCTION_ARGS) {

  static char *b64 = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                     "abcdefghijklmnopqrstuvwxyz"
                     "0123456789./";	
  text        *password = PG_GETARG_TEXT_P(0);
  int32       s_type    = PG_GETARG_INT32(1);
  text        *tcrypted;
  char        *crypted;
  static char salt[] = "$1$12345678$";
  char        *slt;
  size_t      len;
  
  slt = salt + 3;
  while (*slt != '$') {
    *slt++ = b64[(int)(64.0 * rand() / (RAND_MAX + 1.0))];
  }
  if (s_type == 0) {
    slt = salt + 3;
  } else {
    slt = salt;
  }
  crypted = crypt(VARDATA(password), slt);
  len = strlen(crypted) + VARHDRSZ;
  tcrypted = (text *)palloc(len);
  VARATT_SIZEP(tcrypted) = len;
  memcpy(VARDATA(tcrypted), crypted, len - VARHDRSZ);
  PG_RETURN_TEXT_P(tcrypted);
}

