#include <signal.h>
typedef void (*sighandler_t)(int);

#include "sighandler.h"

void (*__saved_signal_handlers[64])(int);

void sh_set_handlers(uint64 trap, uint64 ignore, sighandler_t handler) {

	int sig;

	for (sig = 1; sig < 64; sig++) {
		__saved_signal_handlers[sig] = NULL;
		if (trap & 1) {
			__saved_signal_handlers[sig] = signal(sig, handler);
		} else if (ignore & 1) {
			__saved_signal_handlers[sig] = signal(sig, SIG_IGN);
		}
		ignore >>= 1;
		trap >>= 1;
	}
}

void sh_restore_handlers(void) {

	int sig;

	for (sig = 1; sig < 64; sig++) {
		if (__saved_signal_handlers[sig]) {
			signal(sig, __saved_signal_handlers[sig]);
		}
	}
}
