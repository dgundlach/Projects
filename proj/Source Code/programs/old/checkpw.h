#define CHECKPW_NO_USER			-30
#define CHECKPW_PW_EXPIRED		-31
#define CHECKPW_PW_MANUALLY_EXPIRED	-32
#define CHECKPW_PW_TOO_OLD		-33
#define CHECKPW_PW_BAD			-34
#define CHECKPW_PW_OK			0

int checkpw(char *, char *);
