#define _XOPEN_SOURCE
#include <time.h>
#include <postgres.h>
#include <utils/datetime.h>
#include <utils/date.h>
#include <fmgr.h>
#include <executor/spi.h>
#include <commands/trigger.h>
#include <stdlib.h>
#include <unistd.h>
#include "funcs.h"
#include "triggers.h"

#define PRORATE		7

PG_FUNCTION_INFO_V1(change_user);

Datum change_user(PG_FUNCTION_ARGS) {

  bool        isnull;
  int         attnum;
  int         yr;
  int         mo;
  int         da;
  char        *q, *p, *d;
  bool        alreadyconnected = FALSE;
  char        *opasswd;
  char        *npasswd;
  char        *oemail;
  char        *nemail;
  int32       oexpire;
  int32       nexpire;
  char        olocked;
  char        nlocked;
  DateADT     date;
  char        sep = ' ';
  static char query[256];
  triginfo    ti;
  int         context = TRIG_ROW | TRIG_AFTER | TRIG_UPDATE;

  init_trigger(&ti, "change_user", fcinfo, context);
		    
  if (SPI_connect() == SPI_ERROR_CONNECT)
    alreadyconnected = TRUE;

  q = query;
  q += 1 + sprintf (q, "update users set ");

  attnum = SPI_fnumber(ti.tupdesc, "password");
  npasswd = DatumGetCString(SPI_getbinval(ti.newtuple, ti.tupdesc, attnum,
                            &isnull));
  opasswd = DatumGetCString(SPI_getbinval(ti.oldtuple, ti.tupdesc, attnum,
                            &isnull));
  if (strcmp(opasswd, npasswd)) {
    if (isnull || !strcmp(npasswd, "!!")) {
      q += 1 + sprintf(q, "crypt = '!!'");
    } else {
      q += 1 + sprintf(q, "crypt = '%s'", crypt(npasswd,salt(1)));
    }
    sep = ',';
  }
  
  attnum = SPI_fnumber(ti.tupdesc, "expire_interval");
  nexpire = DatumGetInt32(SPI_getbinval(ti.newtuple, ti.tupdesc, attnum,
                          &isnull));
  oexpire = DatumGetInt32(SPI_getbinval(ti.oldtuple, ti.tupdesc, attnum,
                          &isnull));
  if (oexpire != nexpire) {
    attnum = SPI_fnumber(ti.tupdesc, "billing_start");
    date = DatumGetDateADT(SPI_getbinval(ti.newtuple, ti.tupdesc, attnum,
                           &isnull));
    j2date(date + date2j(2000, 1, 1), &yr, &mo, &da);
    mo += nexpire;
    yr += nexpire / 12;
    mo %= 12;
    q += 1 + sprintf(q, "%cexpire_date = '%04d-%02d-%02d'",sep, yr, mo, da);
    sep = ',';
  }
  
  attnum = SPI_fnumber(ti.tupdesc, "email");
  nemail = DatumGetCString(SPI_getbinval(ti.newtuple, ti.tupdesc, attnum,
                           &isnull));
  oemail = DatumGetCString(SPI_getbinval(ti.oldtuple, ti.tupdesc, attnum,
                           &isnull));
  if (strcmp(oemail, nemail)) {
    q += 1 + sprintf(q, "%clogin = '", sep);
    p = nemail;
    while (*p != '@') {
      *q++ = *p++;
    }
    q += 1 + sprintf(q, "',domain = '");
    p++;
    d = p;
    while (*p) {
      *q++ = *p++;
    }
    q += 1 + sprintf(q, "',home = '");
    p = d;
    while (*p) {
      *q++ = *p++;
    }
    *q++ = '/';
    p = nemail;
    while (*p != '@') {
      *q++ = *p++;
    }
    *q++ = '\'';
    sep = ',';
  }
  
  attnum = SPI_fnumber(ti.tupdesc, "locked");
  nlocked = DatumGetChar(SPI_getbinval(ti.newtuple, ti.tupdesc, attnum,
                         &isnull));
  olocked = DatumGetChar(SPI_getbinval(ti.oldtuple, ti.tupdesc, attnum,
                         &isnull));
  if (olocked != nlocked) {
    q += 1 + sprintf(q, "%cactive = '", sep);
    if (nlocked == 'Y' || nlocked == 'y') {
      *q++ = 'N';
    } else {
      *q++ = 'Y';
    }
    *q++ = '\'';
    sep = ',';
  }
  if (sep == ',') {
    q += 1 + sprintf(q, " where login = '");
    p = oemail;
    while (*p != '@') {
      *q++ = *p++;
    }
    q += 1 + sprintf(q, "' and domain = '");
    p++;
    while (*p) {
      *q++ = *p++;
    }
    *q++ = '\'';
    *q++ = ';';
    *q++ = '\0';
    if (SPI_exec(query, 1) != SPI_OK_SELECT)
      elog(ERROR, "change_user: error updatinf user record.");
  }
  if (!alreadyconnected)
    SPI_finish();
  return PointerGetDatum(ti.newtuple);
}
