/*
 * nss_pg_int.h
 *
 */

#ifndef _NSS_PG_INT_H
#define _NSS_PG_INT_H	1

/*
 * debug level : 
 * 0 : nothing
 * 1 : only connection/query errors
 * 2 : zero or more than one entry found
 * 3 : connection/disconnection/query request
 * 4 : well done connection/query
 * 5 : query string
 */

#define DEBUG	0

/*
 * set MAXSERVERS to the number of database servers you wish
 * to define in the configuration files.
 */

#define MAXSERVERS	10

#define CONF_FILE	"/etc/nss-pgsql.conf"
#define ROOT_CONF_FILE	"/etc/nss-pgsql-root.conf"

/* The names of the tables that the virtual user fields are stored in. */
#define USER_TABLE	"passwd"
#define SHADOW_TABLE	"shadow"
#define GROUP_TABLE	"groups"

/* The fields we are interested in from the user table */
#define PW_NAME_FIELD	"username"
#define PW_PASSWD_FIELD	"passwd"
#define PW_UID_FIELD	"uid"
#define PW_GID_FIELD	"gid"
#define PW_GECOS_FIELD	"gecos"
#define PW_DIR_FIELD	"home"
#define PW_SHELL_FIELD	"shell"

/* The fields we are interested in from the shadow table */
#define SP_NAME_FIELD	"username"
#define SP_PASSWD_FIELD	"passwd"
#define SP_LSTCHG_FIELD	"lastchange"
#define SP_MIN_FIELD	"minchange"
#define SP_MAX_FIELD	"maxchange"
#define SP_WARN_FIELD	"warn"
#define SP_INACT_FIELD	"inact"
#define SP_EXPIRE_FIELD	"expire"
#define SP_FLAG_FIELD	"flag"

/* The fields we are interested in from the group table */ 
#define GR_NAME_FIELD	"groupname"
#define GR_PASSWD_FIELD	"grppasswd"
#define GR_GID_FIELD	"gid"
#define GR_MEM_FIELD	"members"

/* Size of the buffer to use for storing various strings. */
#define BUFFER		512

void _nss_pgsql_log (int, const char *,...);

#endif
