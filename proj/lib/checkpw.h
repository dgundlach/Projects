#define CHECKPW_PW_OK				0
#define CHECKPW_NO_USER				1
#define CHECKPW_PW_EXPIRED			2
#define CHECKPW_PW_MANUALLY_EXPIRED	3
#define CHECKPW_PW_TOO_OLD			4
#define CHECKPW_PW_DISABLED			5
#define CHECKPW_PW_BAD				6

typedef void *(*getuser_f)(char *);
int checkpw(char *, char *, getuser_f, getuser_f);

#ifndef _CHECKPW_SOURCE
#define _CHECKPW_SOURCE
extern *checkpw_errstr[];
#endif
