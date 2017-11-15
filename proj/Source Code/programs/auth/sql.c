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
#include "sql.h"
#include "funcs.h"

char sql[512];
time_t now;
FILE *f;
char **envstrings = NULL;

#define SLOT_ALLOC	5

typedef struct {
  SQLResult Res;
  SQLResultList *Prev, *Next;
} SQLResultList;

static PGresult **PGresults = NULL;
static PGconn **PGconns = NULL;
static SQLResultList **SQLrl = NULL;
static int PGnslots = 0;
static int PGslotsused = 0;

void SQLExitNicely (int code, char *message, ...) {

  va_list ap;
  int slot;

 /*
  * Clear out all query results and close all database connections.
  */

  for (slot = 0; slot < PGnslots; slot++) {
    if (PGresults[slot]) PQclear(PGresults[slot]);
    if (PGconns[slot]) PQfinish(PGconns[slot]);
  }

 /*
  * Display the error message and exit.
  */

  if (message) {
    va_start(ap, message);
    vfprintf(stderr,message,ap);
    va_end(ap);
  }
  exit(code);
}

int SQLConnect(char *connstr) {

  PGconn *conn;
  int slot;

 /*
  * If no slots have been allocated, or all of them are full, allocate more.
  */

  if (PGslotsused == PGnslots) {
    PGnslots += SLOT_ALLOC;

   /*
    * Allocate the connection slots and zero them out.
    */

    PGconns = realloc(PGconns,sizeof(PGconn *) * PGnslots);
    if (!PGconns) SQLExitNicely("Out of memory.\n");
    bzero(PGconns + (sizeof(PGconn *) * (PGnslots - SLOT_ALLOC)), 
		sizeof(PGconn *) * SLOT_ALLOC);

   /*
    * Allocate the result slots and zero them out.
    */

    PGresults = realloc(PGresults,sizeof(PGresult *) * PGnslots);
    if (!PGresults) SQLExitNicely("Out of memory.\n");
    bzero(PGresults + (sizeof(PGresult *) * (PGnslots - SLOT_ALLOC)),
		sizeof(PGresult *) * SLOT_ALLOC);

   /*
    * Allocate the tuple indices.
    */

    PGtupleinds = realloc(PGtupleinds,sizeof(SQLResultList *) * PGnslots);
    if (!PGtupleinds) SQLExitNicely("Out of memory.\n");
    bzero(PGtupleinds + (sizeof(SQLResultList *) * (PGnslots - SLOT_ALLOC)),
		sizeof(SQLResultList *) * SLOT_ALLOC);
  }

 /*
  * Search for an empty slot.  If we don't find one, I've made a logic error.
  */

  for (slot = 0; slot < PGnslots; slot++) {
    if (!PGconns[slot]) break;
  }
  if (slot == PGnslots) {
    SQLExitNicely("\nLogic error in computing free slot. :(\n\n");
  }

 /*
  * Connect to the database.
  */

  conn = PQconnectdb(connstr);
  if ((conn == NULL) || (PQstatus(conn) == CONNECTION_BAD)) {
    PQfinish(conn);
    return -1;
  }

 /*
  * Save the connection info and return the slot used.
  */

  PGconns[slot] = conn;
  return slot;
}

int SQLDisconnect(int slot) {

 /*
  * If the slot exists and is open, clean up any transactions, close the
  * connection, and clear the slots.
  */

  if ((slot >= 0) && (slot < PGnslots) && PGconns[slot]) {
    if (PGresults[slot]) PQclear(PGresults[slot]);
    PQfinish(PGconns[slot]);
    PGconns[slot] = NULL;
    PGresults[slot] = NULL;
    return 0;
  }
  return -1;
}

int SQLExec(int slot, char *sql) {

  if ((slot >= 0) && (slot < PGnslots) && PGconns[slot]) {
    if (PGresults[slot]) PQclear(PGresults[slot]);
    PGresults[slot] = PQexec(PGconns[slot],sql);
    PGtupleinds[slot] = 0;
    return 0;
  }
  return -1;
}

char **SQLGetNext (SQLResult *res) {

  char **field_index = NULL;
  char *data;
  int i;
  int dsize = 0;
  
  field_index = malloc((sizeof (char *)) * (res->NFields + 1)); 
  for (i=0; i<res->NFields; i++) {
    dsize += PQgetlength((PGresult *) (res->Result),res->Next_Tuple,i);
  }
  data = malloc(dsize);
  for (i=0; i<res->NFields; i++) {
    field_index[i] = data;
    strcpy(data,PQgetvalue((PGresult *) (res->Result),res->Next_Tuple,i));
    data += PQgetlength((PGresult *) (res->Result),res->Next_Tuple,i);
  }
  field_index[res->NFields] = NULL;
  res->NextTuple++;
  return field_index;
}

SQLResult **SQLRetrieve(int slot, char *table, char **fields, char *cond, ...) {


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


