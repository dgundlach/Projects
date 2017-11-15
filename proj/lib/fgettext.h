#define FGETTEXT_FAILURE		0
#define FGETTEXT_SUCCESS		1
#define FGETTEXT_EOF			-1
#define FGETTEXT_NOEXIST		-2
#define FGETTEXT_INVALID_INPUT	-3
#define FGETTEXT_OOM			-4

int fgettext(char **, int *, char *);
int fgettext_close(char *);
int fgettext_single(char **, int *, char *);
