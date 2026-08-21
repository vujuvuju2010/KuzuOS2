/* Syscall wrappers for libc - bridges z_syscalls naming */
#include "z_syscalls.h"

/* Wrapper functions that libc expects */

int open(const char *filename, int flags, ...)
{
    return z_open(filename, flags);
}

int close(int fd)
{
    return z_close(fd);
}

ssize_t read(int fd, void *buf, size_t count)
{
    return z_read(fd, buf, count);
}

ssize_t write(int fd, const void *buf, size_t count)
{
    return z_write(fd, buf, count);
}

off_t lseek(int fd, off_t offset, int whence)
{
    return z_lseek(fd, offset, whence);
}

/* Malloc wrappers - use libc malloc/free */
void* kmalloc(size_t size)
{
    /* Will be defined in libc */
    extern void* malloc(size_t size);
    return malloc(size);
}

void kfree(void* ptr)
{
    /* Will be defined in libc */
    extern void free(void* ptr);
    free(ptr);
}
