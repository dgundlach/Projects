extern int d_log_facility;
extern int d_log_events;
extern char *d_program;
extern char *d_pid_file;
extern char *d_user;
extern void (*dCleanup)(void);

void dSetProgramName(char *, char *);
void dInternalCleanUp(void);
void dSetUser(void);
void dDaemonize(void);
