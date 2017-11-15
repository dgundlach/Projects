typedef void (*sighandler_t)(int);

typedef unsigned long long uint64;

//
// These defines shift the signal id to the proper bit.
//

#define _SIGHUP         (uint64)(1<<(SIGHUP-1))
#define _SIGINT         (uint64)(1<<(SIGINT-1))
#define _SIGQUIT        (uint64)(1<<(SIGQUIT-1))
#define _SIGILL         (uint64)(1<<(SIGILL-1))
#define _SIGTRAP        (uint64)(1<<(SIGTRAP-1))
#define _SIGABRT        (uint64)(1<<(SIGABRT-1))
#define _SIGIOT         (uint64)(1<<(SIGIOT-1))
#define _SIGBUS         (uint64)(1<<(SIGBUS-1))
#define _SIGFPE         (uint64)(1<<(SIGFPE-1))
#define _SIGKILL        (uint64)(1<<(SIGKILL-1))
#define _SIGUSR1        (uint64)(1<<(SIGUSR1-1))
#define _SIGSEGV        (uint64)(1<<(SIGSEGV-1))
#define _SIGUSR2        (uint64)(1<<(SIGUSR2-1))
#define _SIGPIPE        (uint64)(1<<(SIGPIPE-1))
#define _SIGALRM        (uint64)(1<<(SIGALRM-1))
#define _SIGTERM        (uint64)(1<<(SIGTERM-1))
#define _SIGSTKFLT      (uint64)(1<<(SIGSTKFLT-1))
#define _SIGCLD         (uint64)(1<<(SIGCLD-1))
#define _SIGCHLD        (uint64)(1<<(SIGCHLD-1))
#define _SIGCONT        (uint64)(1<<(SIGCONT-1))
#define _SIGSTOP        (uint64)(1<<(SIGSTOP-1))
#define _SIGTSTP        (uint64)(1<<(SIGTSTP-1))
#define _SIGTTIN        (uint64)(1<<(SIGTTIN-1))
#define _SIGTTOU        (uint64)(1<<(SIGTTOU-1))
#define _SIGURG         (uint64)(1<<(SIGURG-1))
#define _SIGXCPU        (uint64)(1<<(SIGXCPU-1))
#define _SIGXFSZ        (uint64)(1<<(SIGXFSZ-1))
#define _SIGVTALRM      (uint64)(1<<(SIGVTALRM-1))
#define _SIGPROF        (uint64)(1<<(SIGPROF-1))
#define _SIGWINCH       (uint64)(1<<(SIGWINCH-1))
#define _SIGPOLL        (uint64)(1<<(SIGPOLL-1))
#define _SIGIO          (uint64)(1<<(SIGIO-1))
#define _SIGPWR         (uint64)(1<<(SIGPWR-1))
#define _SIGSYS         (uint64)(1<<(SIGSYS-1))
#define _SIGUNUSED      (uint64)(1<<(SIGUNUSED-1))

void sh_set_handlers(uint64, uint64, sighandler_t);
void sh_restore_handlers(void);
