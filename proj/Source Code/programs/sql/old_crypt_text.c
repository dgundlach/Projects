#define _XOPEN_SOURCE
#include <stdio.h>
#include <postgres.h>
#include <fmgr.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

static char *b64 = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                   "abcdefghijklmnopqrstuvwxyz"
                   "0123456789./";	

PG_FUNCTION_INFO_V1(crypt_text);

Datum crypt_text(PG_FUNCTION_ARGS) {

  text   *password = PG_GETARG_TEXT_P(0);
  text   *salt = PG_GETARG_TEXT_P(1);
  text   *tcrypted;
  char   *crypted;
  char   salt[2];
  size_t len;
  
  salt[0] = b64[(int)(64.0 * rand() / (RAND_MAX + 1.0))];
  salt[1] = b64[(int)(64.0 * rand() / (RAND_MAX + 1.0))];
  crypted = crypt(VARDATA(password), salt);
  len = strlen(crypted) + VARHDRSZ;
  tcrypted = (text *)palloc(len);
  VARATT_SIZEP(tcrypted) = len;
  memcpy(VARDATA(tcrypted), crypted, len - VARHDRSZ);
  PG_RETURN_TEXT_P(tcrypted);
}

PG_FUNCTION_INFO_V1(make_salt);

Datum make_salt(PG_FUMCTION_ARGS) {

  static char salt[] = "$1$12345678$";
  char        *slt = salt + 3;
  int32       s_type = PG_GETARG_INT32(0);
  int         len;
  text        *rsalt;
  
  while (*slt != '$') {
    *slt++ = b64[(int)(64.0 * rand() / (RAND_MAX + 1.0))];
  }
  if (s_type == 0) {
    len = 2 + VARHDRSZ;
    slt = salt + 3;
  } else {
    len = 12 + VARHDRSZ;
    slt = salt;
  }
  rsalt = (text *)palloc(len);
  VARATT_SIZEP(rsalt) = len;
  memcpy(VARDATA(rsalt), slt, len - VARHDRSZ);
  PG_RETURN_TEXT_P(rsalt);
}

