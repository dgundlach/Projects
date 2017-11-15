#include <pwd.h>
#include <unistd.h>
#include <sys/types.h>
#include <shadow.h>
#include <time.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <pgsql/libpq-fe.h>
#include "radacct.h"
#include "users_sql.h"

char sql[512];
time_t now;
FILE *f;
char **envstrings = NULL;

void set_env_string(char *key, char *value) {

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

char *stringify (char *str) {

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

PGconn *connect_sql (void) {

  PGconn *pgconn;
  char buffer[512];
  char *connstr;
  FILE *conf;

 /*
  * Read the conf file, looking for "host=".  That is the connect string.
  */

  if(!(conf = fopen(CONF_FILE,"r"))) {
    return NULL;
  }
  connstr = NULL;
  fgets(buffer,sizeof(buffer),conf);
  while (!feof(conf)) {
    if (!strncmp(buffer,"host=",5)) {
      if (connstr = strchr(buffer,'\r')) *connstr = '\0';
      if (connstr = strchr(buffer,'\n')) *connstr = '\0';
      connstr = buffer;
      break;
    }
    fgets(buffer,sizeof(buffer),conf);
  }
  if (!connstr) {
    return NULL;
  }
 /*
  * Connect to the database.
  */

  pgconn=PQconnectdb(connstr);
  if ((pgconn == NULL) || (PQstatus(pgconn) == CONNECTION_BAD)) {
    PQfinish(pgconn);
    pgconn = NULL;
  }

  return pgconn;
}

PGconn *disconnect_sql (PGconn *pgconn) {
  
 /*
  * Disconnect from the database.
  */

  PQfinish(pgconn);
  return NULL;
}

PGresult *exec_sql (PGconn *conn, char *sql) {

  PGresult *res;

  res=PQexec(conn,sql);
  return res;
}

char **retrieve_fields (PGresult *res, int tup_num) {

  char **field_index = NULL;
  char *data;
  int i;
  int dsize = 0;

  field_index = malloc((sizeof (char *)) * (PQnfields(res) + 1));
  for (i=0; i<PQnfields(res); i++) {
    dsize += PQgetlength(res,tup_num,i);
  }
  data = malloc(dsize);
  for (i=0; i<PQnfields(res); i++) {
    field_index[i] = data;
    strcpy(data,PQgetvalue(res,tup_num,i));
    data += PQgetlength(res,tup_num,i);
  }
  field_index[PQnfields(res)] = NULL;
  return field_index;
}

char *timestamp(time_t stime) {

  static char ts[40];
  struct tm *stm;
      
  stm = localtime(&stime);
  strftime(ts,sizeof(ts),"%Y-%m-%d %T",stm);
  return ts;
}

int set_cursor (PGconn *conn, char *table, char *cursorname, char **fields,
			char *cond, ...) {

  PGresult *res;
  int sz, p, i;
  char *sqbuf;
  va_list ap;

  sz = sizeof(sql);
  sqbuf = sql;
  p = snprintf(sqbuf,sz,"DECLARE %s CURSOR FOR SELECT ",cursorname);
  sz -= p;
  sqbuf += p;
  for (i=0; fields[i]; i++) {
    if (!i) {
      p = snprintf(sqbuf,sz,"%s",fields[i]);
    } else {
      p = snprintf(sqbuf,sz,",%s",fields[i]);
    }
    sz -= p;
    sqbuf += p;
  }
  p = snprintf(sqbuf,sz," FROM %s",table);
  sz -= p;
  sqbuf += p;
  if (cond) {
    p = snprintf(sqbuf,sz," WHERE ");
    sz -= p;
    sqbuf += p;
    va_start(ap,cond);
    vsnprintf(sqbuf,sz,cond,ap);
    va_end(ap);
  }
  res = exec_sql(conn,sql);
  PQclear(res);
  return 1;
}

char **get_next(PGconn *conn, char *cursorname) {

  PGresult *res;
  char **data = NULL;

  snprintf(sql,sizeof(sql),"SELECT NEXT FROM %s",cursorname);
  res = exec_sql(conn,sql);
  if (PQntuples(res) == 1) {
    data = retrieve_fields(res,0);
  }
  PQclear(res);
  return data;
}

int commit_cursor(PGconn *conn);

  PGresult *res;

  res = exec_sql(conn,"COMMIT");
  PQclear(res);
  return 1;
}

char **retrieve_one(PGconn *conn, char *table, char **fields, char *cond, ...) {

  PGresult *res;
  int sz, p, i;
  char *sqbuf;
  char **data;
  va_list ap;

  sz = sizeof(sql);
  sqbuf = sql;
  p = snprintf(sqbuf,sz,"SELECT ");
  sz -= p;
  sqbuf += p;
  for (i=0; fields[i]; i++) {
    if (!i) {
      p = snprintf(sqbuf,sz,"%s",fields[i]);
    } else {
      p = snprintf(sqbuf,sz,",%s",fields[i]);
    }
    sz -= p;
    sqbuf += p;
  }
  p = snprintf(sqbuf,sz," WHERE ");
  sz -= p;
  sqbuf += p;
  va_start(ap,cond);
  vsnprintf(sqbuf,sz,cond,ap);
  va_end(ap);
  res = exec_sql(conn,sql);
  if (PQntuples(res) == 1) {
    data = retrieve_fields(res,0);
  } else {
    data = NULL;
  }
  PQclear(res);
  return data;
}

int store_data(PGconn *conn, char *table, char **fields, char **user_data) {

  PGresult *res;
  int sz, p, i;
  char *sqbuf;

  sz = sizeof(sql);
  sqbuf = sql;
  p = snprintf(sqbuf,sz,"INSERT INTO %s (",table);
  sz -= p;
  sqbuf += p;
  for (i=0; fields[i]; i++) {
    if (!i) {
      p = snprintf(sqbuf,sz,"%s",fields[i]);
    } else {
      p = snprintf(sqbuf,sz,",%s",fields[i]);
    }
    sz -= p;
    sqbuf += p;
  }
  p = snprintf(sqbuf,sz,") VALUES (");
  sz -= p;
  sqbuf += p;
  for (i=0; user_data[i]; i++) {
    if (!i) {
      p = snprintf(sqbuf,sz,"'%s'",user_data[i]);
    } else {
      p = snprintf(sqbuf,sz,",'%s'",user_data[i]);
    }
    sz -= p;
    sqbuf += p;
  }
  snprintf(sqbuf,sz,")");
  res = exec_sql(conn,sql);
  PQclear(res);
  return 1;
}

int update_data(PGconn *conn, char *table, char **fields, char **user_data,
		const char *condition, ...) {

  PGresult *res;
  int sz, p, i;
  char *sqbuf;
  va_list *ap;

  sz = sizeof(sql);
  sqbuf = sql;
  p = snprintf(sqbuf,sz,"UPDATE %s SET ");
  sz -= p;
  sqbuf += p;
  for (i=0; fields[i]; i++) {
    if (!i) {
      p = snprintf(sqbuf,sz,"%s='%s'",fields[i],user_data[i]);
    } else {
      p = snprintf(sqbuf,sz,",%s='%s'",fields[i],user_data[i]);
    }
    sz -= p;
    sqbuf += p;
  }
  p = snprintf(sqbuf,sz," WHERE ");
  sz -= p;
  sqbuf += p;
  va_start(ap,condition);
  vsnprintf(sqbuf,sz,condition,ap);
  va_end(ap);
  res = exec_sql(conn,sql);
  PQclear(res);
  return 1;
}
