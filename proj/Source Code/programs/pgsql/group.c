#include <bits/libc-lock.h>
#include <errno.h>
#include <pgsql/libpq-fe.h>
#include <nss.h>
#include <grp.h>
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
static pthread_mutex_t _nss_pg_gr_lock;

/* global var : the connection handler */
static PGconn *pgconn_grp = NULL;

static enum nss_status internal_setgrent (void)
{
	char *conn_strings = NULL;
	struct stat st;
	FILE *conf;
	char **server_list;
	char *bindex;
	int lindex;
	int whatsleft;
	int linesize;

	if (!pgconn_grp) {
#if DEBUG>2
		_nss_pgsql_log(LOG_INFO,"connecting");
#endif
		if (stat(CONF_FILE,&st)) {
			return NSS_STATUS_UNAVAIL;
		}
                if (!(conn_strings = malloc(st.st_size))) {
			return NSS_STATUS_UNAVAIL;
		}
                if (!(conf = fopen(CONF_FILE,"r"))) {
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
		while (!pgconn_grp && server_list[lindex]) {
			pgconn_grp=PQconnectdb(server_list[lindex++]);
			if ((pgconn_grp == NULL) || (PQstatus(pgconn_grp) == CONNECTION_BAD)) { 
				PQfinish(pgconn_grp);
				pgconn_grp=NULL;
			}
		}
    
		if ((pgconn_grp == NULL) || (PQstatus(pgconn_grp) == CONNECTION_BAD)) { 
#if DEBUG>0
			_nss_pgsql_log(LOG_ERR,"%s",PQerrorMessage(pgconn_grp));
#endif
			PQfinish(pgconn_grp);
			pgconn_grp=NULL;
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


static void internal_endgrent (void)
{
	PGresult *pgres;

	if (pgconn_grp)
	{
		pgres=PQexec(pgconn_grp,"CLOSE grpcursor");
		PQclear(pgres);		
		pgres=PQexec(pgconn_grp,"COMMIT");
		PQclear(pgres);		
		PQfinish(pgconn_grp);
		pgconn_grp = NULL;
#if DEBUG>2
		_nss_pgsql_log(LOG_INFO,"connection closed");
#endif
	}
}


enum nss_status _nss_pgsql_setgrent (void)
{
	enum nss_status status;
	PGresult *pgres;
	char query[BUFFER]="";

	__libc_lock_lock (_nss_pg_gr_lock);
	status = internal_setgrent ();
	if (status == NSS_STATUS_SUCCESS)
	{
		pgres=PQexec(pgconn_grp,"BEGIN");
		if (!pgres || PQresultStatus(pgres) != PGRES_COMMAND_OK)
		{
#if DEBUG>0
			_nss_pgsql_log(LOG_ERR,"%s",PQerrorMessage(pgconn_grp));
#endif
			PQclear(pgres);
			PQfinish(pgconn_grp);
			pgconn_grp=NULL;
			return NSS_STATUS_UNAVAIL;
		}
		PQclear(pgres);
		snprintf(query,BUFFER,
			"DECLARE grpcursor CURSOR FOR "
			"select %s,%s,%s,%s from %s",
			GR_NAME_FIELD,GR_PASSWD_FIELD,
			GR_GID_FIELD,GR_MEM_FIELD,GROUP_TABLE);
		pgres=PQexec(pgconn_grp,query);
		if (!pgres || PQresultStatus(pgres) != PGRES_COMMAND_OK)
		{
#if DEBUG>0
			_nss_pgsql_log(LOG_ERR,"%s",PQerrorMessage(pgconn_grp));
#endif
			PQclear(pgres);
			PQfinish(pgconn_grp);
			pgconn_grp=NULL;
			return NSS_STATUS_UNAVAIL;
		}
		PQclear(pgres);
	}
	__libc_lock_unlock (_nss_pg_gr_lock);
	return status;
}

enum nss_status _nss_pgsql_endgrent (void)
{
	__libc_lock_lock (_nss_pg_gr_lock);
	internal_endgrent ();
	__libc_lock_unlock (_nss_pg_gr_lock);
	return NSS_STATUS_SUCCESS;
}

char **buildmemberlist (char *members, int count)
{
	static char **memberlist;
	char *comma;

	if ((comma=strchr(members,count + 1)))
	{
		*comma='\0';
		comma++;
		memberlist=buildmemberlist(comma,count+1);
	} else {
		memberlist=realloc(memberlist,(sizeof(char *))*(count+1));
		memberlist[count]=NULL;
	}
	memberlist[count-1]=members;
	return memberlist;
}

enum nss_status fill_group (struct group *grp, char *buffer, 
	size_t bufflen, PGresult *pgres)
{
	enum nss_status status=NSS_STATUS_SUCCESS;
	int remain=bufflen;
	char *tmp, *index=buffer;

	tmp=PQgetvalue(pgres,0,0);
	remain-=strlen(tmp)+1;
	if (remain > 0 ) { 
		grp->gr_name=strcpy(index,tmp);
		index+=strlen(index)+1;
	} else { 
		status = NSS_STATUS_TRYAGAIN;
	}
	tmp=PQgetvalue(pgres,0,1);
	remain-=strlen(tmp)+1;
	if (remain > 0 ) { 
		grp->gr_passwd=strcpy(index,tmp);
		index+=strlen(index)+1;
	} else { 
		status = NSS_STATUS_TRYAGAIN;
	}
	grp->gr_gid=atoi(PQgetvalue(pgres,0,2));
	tmp=PQgetvalue(pgres,0,3);
	remain-=strlen(tmp)+1;
        grp->gr_mem=NULL;
/*
	if (remain > 0 ) {
		strcpy(index,tmp);
		if (strlen(index)) {
			grp->gr_mem=buildmemberlist(index,1);
		} else {
			grp->gr_mem=NULL;
		}
	} else { 
		status = NSS_STATUS_TRYAGAIN;
	}
*/
	return status;
}

static enum nss_status gr_lookup (const char *name, const char *type, 
	struct group *grp, char *buffer, size_t bufflen, int *errnop)
{
	enum nss_status status;
	PGresult *pgres = NULL;
	char query[BUFFER]="";
  
	if (!strncmp("gid",type,3)) {
#if DEBUG>2
		_nss_pgsql_log(LOG_INFO,"getgrgid(%s)",name);
#endif
		snprintf(query,BUFFER,
			"select %s,%s,%s,%s from %s where %s = '%s'",
			GR_NAME_FIELD,GR_PASSWD_FIELD,GR_GID_FIELD,
			GR_MEM_FIELD,GROUP_TABLE,GR_GID_FIELD,name);
	}
  
	if (!strncmp("nam",type,3)) {
#if DEBUG>2
		_nss_pgsql_log(LOG_INFO,"getgrnam(%s)",name);
#endif
		snprintf(query,BUFFER,
			"select %s,%s,%s,%s from %s where %s = '%s'",
			GR_NAME_FIELD,GR_PASSWD_FIELD,GR_GID_FIELD,
			GR_MEM_FIELD,GROUP_TABLE,GR_NAME_FIELD,name);

	}
  
	if (!strncmp("ent",type,3)) {
#if DEBUG>2
		_nss_pgsql_log(LOG_INFO,"getgrent(%s)",name);
#endif
		snprintf(query,BUFFER,"FETCH NEXT from grpcursor");
}

	if (strlen(query)==0) { 
		return NSS_STATUS_NOTFOUND;
	}
  
	status = internal_setgrent ();
	if (status != NSS_STATUS_SUCCESS) {
		return status;
	}

#if DEBUG>4
	_nss_pgsql_log(LOG_DEBUG,"%s",query);
#endif
  
	pgres=PQexec(pgconn_grp,query);

	if (PQresultStatus(pgres) == PGRES_TUPLES_OK ) {
		if ( PQntuples(pgres) == 1 ) {
#if DEBUG>3
			_nss_pgsql_log(LOG_INFO,"one entry found",query);
#endif
			status=fill_group(grp,buffer,bufflen,pgres);
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
/*	internal_endgrent();    */
  
	return status;
}

enum nss_status _nss_pgsql_getgrent_r (struct group *grp, 
		char *buffer, size_t buflen, int *errnop)
{
	enum nss_status status;

	__libc_lock_lock (_nss_pg_gr_lock);
	status = gr_lookup (NULL, "ent", grp, buffer, buflen, errnop);
	__libc_lock_unlock (_nss_pg_gr_lock);
	return status;
}

enum nss_status _nss_pgsql_getgrnam_r (const char *name, 
		struct group *grp, char *buffer, size_t buflen, int *errnop)
{
	enum nss_status status;

	__libc_lock_lock (_nss_pg_gr_lock);
	status = gr_lookup (name, "nam", grp, buffer, buflen, errnop);
//	internal_endgrent();
	__libc_lock_unlock (_nss_pg_gr_lock);
	return status;
}

enum nss_status _nss_pgsql_getgrgid_r (gid_t gid, struct group *grp,
		char *buffer, size_t buflen, int *errnop)
{
	enum nss_status status = NSS_STATUS_UNAVAIL;
	/* We will probably never have a gid_t with more than 64 bits.  */
        char gidstr[21];	

	snprintf (gidstr, sizeof gidstr, "%d", gid);
	__libc_lock_lock (_nss_pg_gr_lock);
	status = gr_lookup (gidstr, "gid", grp, buffer, buflen, errnop);
//	internal_endgrent();
	__libc_lock_unlock (_nss_pg_gr_lock);
	return status;
}
