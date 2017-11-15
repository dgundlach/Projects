#include <bits/libc-lock.h>
#include <errno.h>
#include <pgsql/libpq-fe.h>
#include <nss.h>
#include <shadow.h>
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
static pthread_mutex_t _nss_pg_sp_lock;

/* global var : the connection handler */
static PGconn *pgconn_spwd = NULL;

static enum nss_status internal_setspent (void)
{
	char *conn_strings = NULL;
	struct stat st;
	FILE *conf;
	char **server_list;
	char *bindex;
	int lindex;
	int whatsleft;
	int linesize;

	if (!pgconn_spwd) {
#if DEBUG>2
		_nss_pgsql_log(LOG_INFO,"connecting");
#endif
		if (stat(ROOT_CONF_FILE,&st)) {
			return NSS_STATUS_UNAVAIL;
		}
                if (!(conn_strings = malloc(st.st_size))) {
			return NSS_STATUS_UNAVAIL;
		}
                if (!(conf = fopen(ROOT_CONF_FILE,"r"))) {
			return NSS_STATUS_UNAVAIL;
		}
		if (!(server_list = malloc((sizeof (char *))
				* (MAXSERVERS + 1)))) {
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
		while (!pgconn_spwd && server_list[lindex]) {
			pgconn_spwd=PQconnectdb(server_list[lindex++]);
			if ((pgconn_spwd == NULL) || (PQstatus(pgconn_spwd) == CONNECTION_BAD)) {
				PQfinish(pgconn_spwd);
				pgconn_spwd = NULL;
			}
		}

		if ((pgconn_spwd == NULL) || (PQstatus(pgconn_spwd) == CONNECTION_BAD)) { 
#if DEBUG>0
			_nss_pgsql_log(LOG_ERR,"%s",PQerrorMessage(pgconn_spwd));
#endif
			PQfinish(pgconn_spwd);
			pgconn_spwd=NULL;
			return NSS_STATUS_UNAVAIL;
		} else {
#if DEBUG>3
			_nss_pgsql_log(LOG_INFO,"connected");
#endif 
		} 
	} else {
#if DEBUG>2
		_nss_pgsql_log(LOG_INFO,"already connected");
#endif
  	}
  
	return NSS_STATUS_SUCCESS;
}


static void internal_endspent (void)
{
	PGresult *pgres;

	if (pgconn_spwd)
	{
		pgres=PQexec(pgconn_spwd,"CLOSE spwcursor");
		PQclear(pgres);
		pgres=PQexec(pgconn_spwd,"COMMIT");
		PQclear(pgres);
		PQfinish(pgconn_spwd);
		pgconn_spwd = NULL;
#if DEBUG>2
		_nss_pgsql_log(LOG_INFO,"connection closed");
#endif
	}
}


enum nss_status _nss_pgsql_setspent (void)
{
	enum nss_status status;
	PGresult *pgres;
	char query[BUFFER]="";

	__libc_lock_lock (_nss_pg_sp_lock);
	status = internal_setspent ();
        if (status == NSS_STATUS_SUCCESS) {
		pgres=PQexec(pgconn_spwd,"BEGIN");
		if (!pgres || PQresultStatus(pgres) != PGRES_COMMAND_OK)
		{
#if DEBUG>0
			_nss_pgsql_log(LOG_ERR,"%s",PQerrorMessage(pgconn_spwd));
#endif
			PQclear(pgres);
			PQfinish(pgconn_spwd);
			pgconn_spwd=NULL;
			return NSS_STATUS_UNAVAIL;
		}
		PQclear(pgres);
		snprintf(query,BUFFER,
			"DECLARE spwcursor CURSOR FOR "
			"select %s,%s,%s,%s,%s,%s,%s,%s,%s from %s",
			SP_NAME_FIELD,SP_PASSWD_FIELD,SP_LSTCHG_FIELD,
			SP_MIN_FIELD,SP_MAX_FIELD,SP_WARN_FIELD,
			SP_INACT_FIELD,SP_EXPIRE_FIELD,SP_FLAG_FIELD,
			SHADOW_TABLE);
		pgres=PQexec(pgconn_spwd,query);
		if (!pgres || PQresultStatus(pgres) != PGRES_COMMAND_OK)
		{
#if DEBUG>0
			_nss_pgsql_log(LOG_ERR,"%s",PQerrorMessage(pgconn_spwd));
#endif
			PQclear(pgres);
			PQfinish(pgconn_spwd);
			pgconn_spwd=NULL;
			return NSS_STATUS_UNAVAIL;
		}
		PQclear(pgres);
	}
	__libc_lock_unlock (_nss_pg_sp_lock);
	return status;
}

enum nss_status _nss_pgsql_endspent (void)
{
	__libc_lock_lock (_nss_pg_sp_lock);
	internal_endspent ();
	__libc_lock_unlock (_nss_pg_sp_lock);
	return NSS_STATUS_SUCCESS;
}

enum nss_status fill_shadow (struct spwd *sp, char *buffer,
		size_t bufflen, PGresult *pgres)
{
	enum nss_status status=NSS_STATUS_SUCCESS;
	int remain=bufflen;
	char *tmp,*index=buffer;

	tmp=PQgetvalue(pgres,0,0);
	remain-=strlen(tmp)+1;
	if (remain > 0 ) { 
		sp->sp_namp=strcpy(index,tmp);
		index+=strlen(index)+1;
	} else { 
		status = NSS_STATUS_TRYAGAIN;
	}
	tmp=PQgetvalue(pgres,0,1);
	remain-=strlen(tmp)+1;
	if (remain > 0 ) { 
		sp->sp_pwdp=strcpy(index,tmp);
		index+=strlen(index)+1;
	} else { 
		status = NSS_STATUS_TRYAGAIN;
	}
	sp->sp_lstchg=atoi(PQgetvalue(pgres,0,2));
	sp->sp_min=atoi(PQgetvalue(pgres,0,3));
	sp->sp_max=atoi(PQgetvalue(pgres,0,4));
	sp->sp_warn=atoi(PQgetvalue(pgres,0,5));
	sp->sp_inact=atoi(PQgetvalue(pgres,0,6));
	sp->sp_expire=atoi(PQgetvalue(pgres,0,7));
	sp->sp_flag=atoi(PQgetvalue(pgres,0,8));
	return status;
}

static enum nss_status sp_lookup (const char *name, const char *type,
		struct spwd *sp, char *buffer, size_t bufflen, int *errnop)
{
	enum nss_status status;
	PGresult *pgres = NULL;
	char query[BUFFER]="";
  
#if DEBUG>2
	_nss_pgsql_log(LOG_INFO,"getspnam(%s)",name);
#endif
	snprintf(query,BUFFER,
		"select %s,%s,%s,%s,%s,%s,%s,%s,%s from %s where %s = '%s'",
		SP_NAME_FIELD,SP_PASSWD_FIELD,SP_LSTCHG_FIELD,
		SP_MIN_FIELD,SP_MAX_FIELD,SP_WARN_FIELD,SP_INACT_FIELD,
		SP_EXPIRE_FIELD,SP_FLAG_FIELD,SHADOW_TABLE,
		SP_NAME_FIELD,name);
  
	if (!strncmp("ent",type,3)) {
#if DEBUG>2
		_nss_pgsql_log(LOG_INFO,"getspent(%s)",name);
#endif
		snprintf(query,BUFFER,"FETCH NEXT from spwcursor");
	}

	if (strlen(query)==0) { 
		return NSS_STATUS_NOTFOUND;
	}
  
	status = internal_setspent ();
	if (status != NSS_STATUS_SUCCESS) {
		return status;
	}

#if DEBUG>4
	_nss_pgsql_log(LOG_DEBUG,"%s",query);
#endif
  
	pgres=PQexec(pgconn_spwd,query);

	if (PQresultStatus(pgres) == PGRES_TUPLES_OK ) {
		if ( PQntuples(pgres) == 1 ) {
#if DEBUG>3
			_nss_pgsql_log(LOG_INFO,"one entry found",query);
#endif
			status=fill_shadow(sp,buffer,bufflen,pgres);
		} else { 
			/* zero or more than one entry found */
#if DEBUG>1
			_nss_pgsql_log(LOG_WARNING,"%d entry found",PQntuples(pgres));
#endif
			status = NSS_STATUS_NOTFOUND;
		} 
	} else { 
		/* query errors */
#if DEBUG>0
		_nss_pgsql_log(LOG_ERR,"%s",PQresultErrorMessage(pgres));
#endif 
		status = NSS_STATUS_NOTFOUND;
	}
  
	PQclear(pgres);
/*	internal_endspent(); */
  
	return status;
}

enum nss_status _nss_pgsql_getspent_r (struct spwd *sp,
		char *buffer, size_t buflen, int *errnop)
{
	enum nss_status status;

	__libc_lock_lock (_nss_pg_sp_lock);
	status = sp_lookup (NULL, "ent", sp, buffer, buflen, errnop);
	__libc_lock_unlock (_nss_pg_sp_lock);
	return status;
}

enum nss_status _nss_pgsql_getspnam_r (const char *name, 
		struct spwd *sp, char *buffer, size_t buflen, int *errnop)
{
	enum nss_status status;

	__libc_lock_lock (_nss_pg_sp_lock);
	status = sp_lookup (name, "nam", sp, buffer, buflen, errnop);
//	internal_endspent();
	__libc_lock_unlock (_nss_pg_sp_lock);
	return status;
}
