// uname.c - Print system information
#include "z_syscalls.h"

void _start(void) {
    z_write(1, "KuzuOS 1.0 (i386)\n", 18);
    z_exit(0);
}
