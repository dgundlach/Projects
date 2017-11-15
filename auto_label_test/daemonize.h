void daemonize_signal_handler(int);
void daemon_clean_up(void);
void daemonize_set_user(char *);
void daemonize(char *, char *, int, void (*)(int));
