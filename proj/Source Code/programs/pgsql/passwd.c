#include <bits/libc-lock.h>
#include <errno.h>
#include <pgsql/libpq-fe.h>
#include <nss.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "setup.h"

/* Locks the static variables in this file.  */
static pthread_mutex_t _nss_pg_lock;

/* global var : the connection handler */
static PGconn *pgconn = NULL;

static enum nss_status internal_setpwent (void)
{
  char *conn_strings = NULL;
  struct stat st;
  FILE *conf;
  char **server_list;
  char *bindex;
  int lindex;
  int whatsleft;
  int linesize;

  if (!pgconn) {
    if (stat(CONF_FILE,&st)) {
      return NSS_STATUS_UNAVAIL;
    }
    if (!(conn_strings = malloc(st.st_size))) {
      return NSS_STATUS_UNAVAIL;
    }
    if (!(conf = fopen(CONF_FILE,"r"))) {
      return NSS_STATUS_UNAVAIL;
    }
    if (!(server_list = malloc((sizeof (char *)) * (MAXSERVERS + 1)))) {
      return NSS_STATUS_UNAVAIL;
    }
    bindex = conn_strings;
    lindex = 0;
    whatsleft = st.st_size;
    fgets(bindex,whatsleft,conf);
    while (!feof(conf) && lindex != MAXSERVERS) {
      server_list[lindex++] = bindex;
      linesize = strlen(bindex);
      whatsleft -= linesize;
      bindex += linesize;
      fgets(bindex,whatsleft,conf);
    }
    server_list[lindex] = NULL;
    fclose(conf);
    lindex = 0;
    while (!pgconn && server_list[lindex]) {
      pgconn=PQconnectdb(server_list[lindex++]);
      if ((pgconn == NULL) || (PQstatus(pgconn) == CONNECTION_BAD)) {
        PQfinish(pgconn);
	pgconn = NULL;
      }
    }
    if ((pgconn == NULL) || (PQstatus(pgconn) == CONNECTION_BAD)) { 
      PQfinish(pgconn);
      pgconn=NULL;
      return NSS_STATUS_UNAVAIL;
    }
  }
  return NSS_STATUS_SUCCESS;
}

static void internal_endpwent (void)
{
  PGresult *pgres;

  if (pgconn) {
    pgres=PQexec(pgconn,"CLOSE pwcursor");
    PQclear(pgres);
    pgres=PQexec(pgconn,"COMMIT");
    PQclear(pgres);
    PQfinish(pgconn);
    pgconn = NULL;
  }
}

enum nss_status _nss_pgsql_setpwent (void)
{
  enum nss_status status;
  PGresult *pgres;
  char query[BUFFER]="";

  __libc_lock_lock (_nss_pg_lock);
  status = internal_setpwent ();
  if (status == NSS_STATUS_SUCCESS) {
    pgres=PQexec(pgconn,"BEGIN");
    if (!pgres || PQresultStatus(pgres) != PGRES_COMMAND_OK) {
      PQclear(pgres);
      PQfinish(pgconn);
      pgconn=NULL;
      return NSS_STATUS_UNAVAIL;
    }
    PQclear(pgres);
    snprintf(query,BUFFER,
        "DECLARE pwcursor CURSOR FOR "
        "select %s,%s,%s,%s,%s,%s,%s from %s",
        PW_NAME_FIELD,PW_PASSWD_FIELD,PW_UID_FIELD,
        PW_GID_FIELD,PW_GECOS_FIELD,PW_DIR_FIELD,
        PW_SHELL_FIELD,USER_TABLE);
    pgres=PQexec(pgconn,query);
    if (!pgres || PQresultStatus(pgres) != PGRES_COMMAND_OK) {
      PQclear(pgres);
      PQfinish(pgconn);
      pgconn=NULL;
      return NSS_STATUS_UNAVAIL;
    }
    PQclear(pgres);
  }
  __libc_lock_unlock (_nss_pg_lock);
  return status;
}

enum nss_status _nss_pgsql_endpwent (void)
{
  __libc_lock_lock (_nss_pg_lock);
  internal_endpwent ();
  __libc_lock_unlock (_nss_pg_lock);
  return NSS_STATUS_SUCCESS;
}

enum nss_status fill_passwd (struct passwd *pwd, char *buffer, 
	size_t bufflen, PGresult *pgres)
{
  enum nss_status status=NSS_STATUS_SUCCESS;
  int remain=bufflen;
  char *tmp,*index=buffer;

  tmp=PQgetvalue(pgres,0,0);
  remain-=strlen(tmp)+1;
  if (remain > 0 ) { 
    pwd->pw_name=strcpy(index,tmp);
    index+=strlen(index)+1;
  } else { 
    status = NSS_STATUS_TRYAGAIN;
  }
  tmp=PQgetvalue(pgres,0,1);
  remain-=strlen(tmp)+1;
  if (remain > 0 ) { 
    pwd->pw_passwd=strcpy(index,tmp);
    index+=strlen(index)+1;
  } else { 
    status = NSS_STATUS_TRYAGAIN;
  }
  pwd->pw_uid=atoi(PQgetvalue(pgres,0,2));
  pwd->pw_gid=atoi(PQgetvalue(pgres,0,3));
  tmp=PQgetvalue(pgres,0,4);
  remain-=strlen(tmp)+1;
  if (remain > 0 ) { 
    pwd->pw_gecos=strcpy(index,tmp);
    index+=strlen(index)+1;
  } else { 
    status = NSS_STATUS_TRYAGAIN;
  }
  tmp=PQgetvalue(pgres,0,5);
  remain-=strlen(tmp)+1;
  if (remain > 0 ) { 
    pwd->pw_dir=strcpy(index,tmp);
    index+=strlen(index)+1;
  } else { 
    status = NSS_STATUS_TRYAGAIN;
  }
  tmp=PQgetvalue(pgres,0,6);
  remain-=strlen(tmp)+1;
  if (remain > 0 ) { 
    pwd->pw_shell=strcpy(index,tmp);
    index+=strlen(index)+1;
  } else { 
    status = NSS_STATUS_TRYAGAIN;
  }
  return status;
}


static enum nss_status lookup (const char *name, const char *type, 
	struct passwd *pwd, char *buffer, size_t bufflen, int *errnop)
{
  enum nss_status status;
  PGresult *pgres = NULL;
  char query[BUFFER]="";
  
  if (!strncmp("uid",type,3)) {
    snprintf(query,BUFFER,
		"select %s,%s,%s,%s,%s,%s,%s from %s where %s = %s",
		PW_NAME_FIELD,PW_PASSWD_FIELD,PW_UID_FIELD,
		PW_GID_FIELD,PW_GECOS_FIELD,PW_DIR_FIELD,
		PW_SHELL_FIELD,USER_TABLE,PW_UID_FIELD,name);
  }
  if (!strncmp("nam",type,3)) {
    snprintf(query,BUFFER,
		"select %s,%s,%s,%s,%s,%s,%s from %s where %s = '%s'",
      		PW_NAME_FIELD,PW_PASSWD_FIELD,PW_UID_FIELD,
		PW_GID_FIELD,PW_GECOS_FIELD,PW_DIR_FIELD,
		PW_SHELL_FIELD,USER_TABLE,PW_NAME_FIELD,name);
  }
  if (!strncmp("ent",type,3)) {
    snprintf(query,BUFFER,"FETCH NEXT from pwcursor");
  }
  if (strlen(query)==0) { 
    return NSS_STATUS_NOTFOUND;
  }
  status = internal_setpwent ();
  if (status != NSS_STATUS_SUCCESS) {
    return status;
  }
  pgres=PQexec(pgconn,query);
  if (PQresultStatus(pgres) == PGRES_TUPLES_OK ) {
    if ( PQntuples(pgres) == 1 ) {
      status = fill_passwd(pwd,buffer,bufflen,pgres);
    } else { 
			/* zero or more than one entry found */
      status = NSS_STATUS_NOTFOUND;
    } 
  } else { 
		/* query errors */
    status = NSS_STATUS_NOTFOUND;
  }
  PQclear(pgres);

/* This is moved to the actual function so that the cursor won't be closed */
/* when sequentially moving through the database                           */
/*	internal_endpwent();   */
  
  return status;
}

enum nss_status _nss_pgsql_getpwent_r (struct passwd *pwd,
		char *buffer, size_t buflen, int *errnop)
{
  enum nss_status status;

  __libc_lock_lock (_nss_pg_lock);
  status = lookup (NULL, "ent", pwd, buffer, buflen, errnop);
  __libc_lock_unlock (_nss_pg_lock);
  return status;
}

enum nss_status _nss_pgsql_getpwnam_r (const char *name, 
		struct passwd *pwd, char *buffer, size_t buflen, int *errnop)
{
  enum nss_status status;

  __libc_lock_lock (_nss_pg_lock);
  status = lookup (name, "nam", pwd, buffer, buflen, errnop);
//  internal_endpwent();
  __libc_lock_unlock (_nss_pg_lock);
  return status;
}

enum nss_status _nss_pgsql_getpwuid_r (uid_t uid, struct passwd *pwd,
		char *buffer, size_t buflen, int *errnop)
{
  enum nss_status status = NSS_STATUS_UNAVAIL;
	/* We will probably never have a gid_t with more than 64 bits.  */
  char uidstr[21];	

  snprintf (uidstr, sizeof uidstr, "%d", uid);
  __libc_lock_lock (_nss_pg_lock);
  status = lookup (uidstr, "uid", pwd, buffer, buflen, errnop);
//  internal_endpwent();
  __libc_lock_unlock (_nss_pg_lock);
  return status;
}
