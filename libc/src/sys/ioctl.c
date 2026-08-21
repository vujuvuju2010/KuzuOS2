/* libc/src/sys/ioctl.c - ioctl syscall wrapper */
#include <stddef.h>
#include "../../../src/z_syscalls.h"

extern int z_ioctl(int fd, int request, void* arg);

int ioctl(int fd, int request, ...) {
    void* arg = NULL;
    /* Extract vararg - simplified */
    __builtin_va_list ap;
    __builtin_va_start(ap, request);
    arg = __builtin_va_arg(ap, void*);
    __builtin_va_end(ap);
    
    return z_ioctl(fd, request, arg);
}
