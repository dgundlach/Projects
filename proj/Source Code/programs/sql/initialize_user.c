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

PG_FUNCTION_INFO_V1(initialize_user);

Datum initialize_user(PG_FUNCTION_ARGS) {

  HeapTuple   rettuple = NULL;
  bool        isnull;
  int         attnum;
  time_t      now;
  struct tm   *tm;
  DateADT     date;
  int         yr;
  int         mo;
  int         da;
  int         period;
  double      credit;
  static char query[1024];
  char        *q, *p, *d;
  char        *email;
  char        *passwd;
  int32       gid;
  bool        alreadyconnected = FALSE;
  triginfo    ti;
  int         context = TRIG_ROW | TRIG_BEFORE | TRIG_INSERT;
  
#define N_ATTRS 6
  Datum       newvals[N_ATTRS];
  int         chattrs[N_ATTRS];
  double      fee;
  
  static int  dim[] = {31, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31, 31};

  init_trigger(&ti, "initialize_user", fcinfo, context);

  rettuple = ti.newtuple;
		    
  if (SPI_connect() == SPI_ERROR_CONNECT)
    alreadyconnected = TRUE;

  attnum = SPI_fnumber(ti.tupdesc, "monthly_fee");
  fee = DatumGetFloat8(SPI_getbinval(rettuple, ti.tupdesc, attnum, &isnull));
  
  chattrs[0] = SPI_fnumber(ti.tupdesc, "added_by");
  if (SPI_exec("select current_user;", 1) != SPI_OK_SELECT)
    elog(ERROR, "initialize_user: error in database backend.");
  newvals[0] = SPI_getbinval(*SPI_tuptable->vals,
                             SPI_tuptable->tupdesc, 1, &isnull);

  chattrs[1] = SPI_fnumber(ti.tupdesc, "billing_interval");
  newvals[1] = Int32GetDatum(0);

  chattrs[2] = SPI_fnumber(ti.tupdesc, "expire_interval");
  newvals[2] = Int32GetDatum(1);

  chattrs[3] = SPI_fnumber(ti.tupdesc, "start_date");
  now = time(NULL);
  tm = localtime(&now);
  newvals[3] = DateADTGetDatum(date2j(tm->tm_year, tm->tm_mon + 1, 
                               tm->tm_mday));

  chattrs[4] = SPI_fnumber(ti.tupdesc, "billing_start");
  date = DatumGetDateADT(SPI_getbinval(rettuple, ti.tupdesc, chattrs[4], 
                         &isnull));
  j2date(date + date2j(2000, 1, 1), &yr, &mo, &period);
  yr = tm->tm_year;
  mo = tm->tm_mon + 1;
  da = tm->tm_mday;
  if (period == 15) {
    da = da - 15;
    if (da < 1) {
      mo = mo - 1;
      if (mo < 1) {
        yr = yr - 1;
      }
      da += dim[mo];
    }
  } else {
    period = 1;
  }
  credit = 0;
  if (da <= (30 - PRORATE)) {
    if (da > PRORATE) {
      credit = (da - 1) * (floor((fee * 100.0) / 30.0) / 100.0);
    }
  } else {
    mo = mo + 1;
    if (mo > 12) {
      mo = 1;
      yr++;
    }
  }
  newvals[4] = DateADTGetDatum(date2j(yr, mo, period));

  chattrs[5] = SPI_fnumber(ti.tupdesc, "credit");
  newvals[5] = Float8GetDatum(credit);

  q = query;
  q += 1 + sprintf(query, "insert into users (login, domain, home, crypt, "
		  "start_date, expire_date, uid, gid) values ('");
  attnum = SPI_fnumber(ti.tupdesc, "email");
  email = DatumGetCString(SPI_getbinval(rettuple, ti.tupdesc, attnum, &isnull));

// Login

  p = email;
  while (*p != '@') {
    *q++ = *p++;
  }
  *q++ = '\'';
  *q++ = ',';

// Domain

  *q++ = '\'';
  p++;
  d = p;
  while (*p) {
    *q++ = *p++;
  }
  *q++ = '\'';
  *q++ = ',';

// Home

  *q++ = '\'';
  p = d;
  while (*p) {
    *q++ = *p++;
  }
  *q++ = '/';
  p = email;
  while (*p != '@') {
    *q++ = *p++;
  }
  *q++ = '\'';
  *q++ = ',';

// Crypt

  attnum = SPI_fnumber(ti.tupdesc, "password");
  passwd = DatumGetCString(SPI_getbinval(rettuple, ti.tupdesc, attnum, 
                           &isnull));
  if (isnull || !strcmp(passwd, "!!")) {
    q += 1 + sprintf(q, "'!!',");
  } else {
    q += 1 + sprintf(q, "'%s',", crypt(passwd,salt(1)));
  }

// Start date, Expire date, Uid, Gid

  attnum = SPI_fnumber(ti.tupdesc, "acctno");
  gid = DatumGetInt32(SPI_getbinval(rettuple, ti.tupdesc, attnum, &isnull));
  sprintf(q, "'%04d-%02d-%02d', '%04d-%02d-%02d', %d, %d);", 
             tm->tm_year, tm->tm_mon + 1, tm->tm_mday,
             yr, mo, period, gid, gid);
  if (SPI_exec(query, 1) != SPI_OK_SELECT)
    elog(ERROR, "initialize_user: error inserting user record.");
  if (!alreadyconnected)
    SPI_finish();
  rettuple = SPI_modifytuple(ti.rel, rettuple, N_ATTRS, chattrs, newvals,
                             NULL);
  if (rettuple == NULL)
    elog(ERROR, "initialize_user: %d returned by SPI_modifytuple.",
                SPI_result); 
  return PointerGetDatum(rettuple);
}















  
