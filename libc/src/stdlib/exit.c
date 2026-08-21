/* libc/src/stdlib/exit.c */
#include <stdlib.h>

extern void z_exit(int status);

void exit(int status) {
    z_exit(status);
    while(1);  /* Never reached */
}

void _exit(int status) {
    exit(status);
}

void abort(void) {
    exit(1);
}

int atexit(void (*function)(void)) {
    /* Simplified: just store one atexit handler */
    /* Real implementation would maintain a list */
    return 0;
}

char* getenv(const char* name) {
    return NULL;  /* No environment in KuzuOS5 yet */
}

int system(const char* command) {
    return -1;  /* Not supported */
}
