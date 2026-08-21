/* libc/src/sys/signal.c - Signal handling */
#include <signal.h>

typedef void (*sighandler_t)(int);
static sighandler_t signal_handlers[64] = {NULL};

sighandler_t signal(int signum, sighandler_t handler) {
    if (signum < 0 || signum >= 64) return SIG_ERR;
    sighandler_t old = signal_handlers[signum];
    signal_handlers[signum] = handler;
    return old;
}

int sigaction(int signum, const struct sigaction *act, struct sigaction *oldact) {
    /* Minimal stub - just track handlers */
    if (signum < 0 || signum >= 64) return -1;
    if (oldact) {
        oldact->sa_handler = signal_handlers[signum];
    }
    if (act) {
        signal_handlers[signum] = act->sa_handler;
    }
    return 0;
}

int pause(void) {
    /* Block indefinitely */
    while(1);
    return 0;
}
