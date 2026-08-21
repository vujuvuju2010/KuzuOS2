// whoami.c - Print current user
#include "z_syscalls.h"

void _start(void) {
    z_write(1, "god\n", 5);// TODO: implement actual user management and permissions
    z_exit(0);
}
