// date.c - Print current date/time
#include "z_syscalls.h"

void _start(void) {
    // For now, just print a placeholder
    z_write(1, "KuzuOS Time\n", 12);
    z_exit(0);
}
