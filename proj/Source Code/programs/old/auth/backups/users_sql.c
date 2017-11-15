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
#include "funcs.h"

char sql[512];
time_t now;
FILE *f;
char **envstrings = NULL;

#define SLOT_ALLOC	5

static PGresult **PGresult_slot = NULL;
static PGconn **PGconn_slot = NULL;
static int PGconn_nslots = 0;
static int PGconn_slotsused = 0;

void SQLExitNicely (char *message, ...) {

  va_list ap;
  int slot;

 /*
  * Clear out all query results and close all database connections.
  */

  for (slot = 0; slot < PGconn_nslots; slot++) {
    if (PGresult_slot[slot]) PQclear(PGresult_slot[slot]);
    if (PGconn_slot[slot]) PQfinish(PGconn_slot[slot]);
  }
  va_start(ap, message);
  vfprintf(stderr,message,ap);
  va_end(ap);
  exit(1);
}

int SQLConnect(char *connstr) {

  PGconn *conn;
  int slot;

  if (PGconn_slotsused == PGconn_nslots) {
    PGconn_nslots += SLOT_ALLOC;
    PGconn_slot = realloc(PGconn_slot,sizeof(PGconn *) * PGconn_nslots);
    if (!PGconn_slot) SQLExitNicely("Out of memory.\n");
    bzero(PGconn_slot + (sizeof(PGconn *) * (PGconn_nslots - SLOT_ALLOC)), 
		sizeof(PGconn *) * SLOT_ALLOC);
    PGresult_slot = realloc(PGresult_slot,sizeof(*PGresult) * PGconn_nslots);
    if (!PGresult_slot) SQLExitNicely("Out of memory.\n");
    bzero(PGresult_slot + (sizeof(PGresult *) * (PGconn_nslots - SLOT_ALLOC)),
		sizeof(PGresult *) * SLOT_ALLOC);
  }
  for (slot = 0; slot < PGconn_nslots; slot++) {
    if (!PGconn_slot[slot]) break;
  }
  if (slot == PGconn_nslots) {
    SQLExitNicely("\nLogic error in computing free slot. :(\n\n");
  }
  conn = PQconnectdb(connstr);
  if ((conn == NULL) || (PQstatus(conn) == CONNECTION_BAD)) {
    PQfinish(conn);
    return -1;
  }
  PGconn_slot[slot] = conn;
  return slot;
}

int SQLDisconnect(int slot) {

  if ((slot > 0) && (slot < PGconn_nslots) && PGconn_slot[slot]) {
    PQfinish(PGconn_slot[slot]);
    PGconn_slot[slot] = NULL;
    return 0;
  }
  return -1;
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

int commit_cursor(PGconn *conn) {

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

