#define GETCONFIG_OK			0
#define GETCONFIG_ERR_OUT_OF_MEMORY	1
#define GETCONFIG_ERR_NO_CONFIG		2
#define GETCONFIG_ERR_BAD_FORMAT	3
#define GETCONFIG_ERR_BAD_KEYWORD	4

struct param_t {
    char *n;
    char **d;
    char *i;
};

#ifndef _COMPILING_GETCONFIG_
static char *getconfig_errstr[] = {
	"",
	"Out of memory.",
	"No configuration file.",
	"Bad configuration file format.",
	"Invalid configuration keyword."
};
#endif

int getconfig(struct param_t *, char *);
