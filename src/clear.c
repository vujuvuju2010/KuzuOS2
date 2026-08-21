// clear.c - Clear screen command via syscall
#include "z_syscalls.h"

#define SYS_IOCTL_SCREEN 200
#define SCREEN_CLEAR 1

static inline int syscall2(int num, int arg1, int arg2) {
    int ret;
    __asm__ volatile("int $0x80" : "=a"(ret) : "a"(num), "b"(arg1), "c"(arg2));
    return ret;
}

void _start(void) {
    syscall2(SYS_IOCTL_SCREEN, SCREEN_CLEAR, 0);
    z_exit(0);
}
