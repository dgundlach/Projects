#define ONE_DAY			86400       /* 60 * 60 * 24 */

#define	USERS_TABLE		"users"
#define USERS_USERNAME		"username"
#define USERS_PASSWD		"passwd"
#define USERS_UID		"uid"
#define USERS_GID		"gid"
#define USERS_COMMENT		"comment"
#define USERS_HOMEDIR		"homedir"
#define USERS_LASTCHANGE	"lastchange"
#define USERS_MINCHANGE		"minchange"
#define USERS_MAXCHANGE		"maxchange"
#define USERS_WARN		"warn"
#define USERS_INACT		"inact"
#define USERS_EXPIRE		"expire"
#define USERS_LOCKDATE		"updatedby"
#define USERS_LOCKEDBY		"updatedate"
#define USERS_LOCKCOMMENT	"updateaction"

#define USERS_SHELL		"shell"
#define USERS_DOMAIN		"domain"
#define USERS_MAILDEST		"maildest"
#define USERS_FLAG		"flag"

char *user_fields[] = {
	USERS_USERNAME,		USERS_PASSWD,		USERS_UID,
	USERS_GID,		USERS_GECOS,		USERS_HOMEDIR,
	USERS_SHELL,		USERS_DOMAIN,		USERS_MAILDEST,
	USERS_LASTCHANGE,	USERS_MINCHANGE,	USERS_MAXCHANGE,
	USERS_WARN,		USERS_INACT,		USERS_EXPIRE,
	USERS_FLAG,		USERS_UPDATEBY,		USERS_UPDATEDATE,
	USERS_UPDATEACTION
};

void set_env_string(char *, char *);
char *stringify(char *);
PGconn *connect_sql(void);
PGconn *disconnect_sql(PGconn *);
char **retrieve_fields(PGresult *, int);
char *timestamp(time_t);
int store_data(PGconn *, char *, char **, char **);
int update_data(PGconn *, char *, char **, char **, const char *, ...);
