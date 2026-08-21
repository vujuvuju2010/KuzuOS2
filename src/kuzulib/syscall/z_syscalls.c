// z_syscalls.c - userspace syscall wrappers
#include "syscall.h" // shall stay

#include "z_asm.h"
#include "z_syscalls.h"

/*
 * Your syscall.h defines things like:
 *   SYS_READ, SYS_WRITE, SYS_OPENAT, SYS_MMAP, SYS_MUNMAP, SYS_MPROTECT, SYS_EXIT ...
 * The macros below use names like SYS_read, SYS_write, SYS_openat, etc.
 * These aliases bridge that naming difference.
 */
#define SYS_read      SYS_READ
#define SYS_write     SYS_WRITE
#define SYS_close     SYS_CLOSE
#define SYS_lseek     SYS_LSEEK
#define SYS_exit      SYS_EXIT
#define SYS_mmap      SYS_MMAP
#define SYS_munmap    SYS_MUNMAP
#define SYS_mprotect  SYS_MPROTECT
#define SYS_openat    SYS_OPENAT

static int errno;

int *z_perrno(void)
{
    return &errno;
}

static long check_error(long rc)
{
    if (rc < 0 && rc > -4096) {
        errno = -rc;
        rc = -1;
    }
    return rc;
}

/*
 * z_syscall is the low-level asm entry point defined in z_syscall.S.
 * It takes:
 *   (long sysno, long arg1, long arg2, long arg3, long arg4, long arg5, long arg6)
 * and returns long.
 *
 * The SYSCALL() macro wraps z_syscall and passes the right SYS_xxx number.
 */
#define SYSCALL(name, ...)  check_error(z_syscall(SYS_##name, __VA_ARGS__))

#define DEF_SYSCALL1(ret, name, t1, a1) \
ret z_##name(t1 a1) \
{ \
    return (ret)SYSCALL(name, a1); \
}

#define DEF_SYSCALL2(ret, name, t1, a1, t2, a2) \
ret z_##name(t1 a1, t2 a2) \
{ \
    return (ret)SYSCALL(name, a1, a2); \
}

#define DEF_SYSCALL3(ret, name, t1, a1, t2, a2, t3, a3) \
ret z_##name(t1 a1, t2 a2, t3 a3) \
{ \
    return (ret)SYSCALL(name, a1, a2, a3); \
}

/*
 * These wrappers are for user mode only.
 * In kernel space, loader_kernel.c provides its own versions.
 */
#ifndef KERNEL_SPACE

DEF_SYSCALL3(int, openat, int, dirfd, const char *, filename, int, flags)
DEF_SYSCALL3(ssize_t, read,  int, fd, void *,        buf,      size_t, count)
DEF_SYSCALL3(ssize_t, write, int, fd, const void *,  buf,      size_t, count)
DEF_SYSCALL1(int,     close, int, fd)
DEF_SYSCALL3(int,     lseek, int, fd, off_t,         off,      int,    whence)
DEF_SYSCALL2(int,     munmap, void *, addr, size_t, length)
DEF_SYSCALL3(int,     mprotect, void *, addr, size_t, length, int, prot)

/* z_exit must be noreturn so GCC doesn't emit code after calls to it */
__attribute__((noreturn)) void z_exit(int status)
{
    z_syscall(SYS_exit, status, 0, 0, 0, 0, 0);
    /* should never reach here, but loop to satisfy noreturn */
    while (1) { __asm__ volatile("hlt"); }
}

int z_open(const char *filename, int flags)
{
    return z_openat(AT_FDCWD, filename, flags);
}

void *z_mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset)
{
    /*
     * On 32-bit x86 Linux there is mmap (old_mmap) and mmap2.
     * mmap2 expects the offset in 4096-byte page units.
     * If SYS_mmap2 is defined, use it; otherwise fall back to SYS_mmap.
     */
#ifdef SYS_mmap2
    return (void *)SYSCALL(mmap2, addr, length, prot, flags, fd, offset >> 12);
#else
    return (void *)SYSCALL(mmap, addr, length, prot, flags, fd, offset);
#endif
}

// Custom KuzuOS syscall wrappers
#define SYS_ioctl_screen    200
#define SYS_draw_pixel      600
#define SYS_getcwd          202
#define SCREEN_CLEAR        1
#define SCREEN_HOME         2
#define SCREEN_SETCURSOR    3

int z_clear_screen(void)
{
    return z_syscall(SYS_ioctl_screen, SCREEN_CLEAR, 0, 0, 0, 0, 0);
}

int z_screen_home(void)
{
    return z_syscall(SYS_ioctl_screen, SCREEN_HOME, 0, 0, 0, 0, 0);
}

int z_set_cursor(int row, int col)
{
    return z_syscall(SYS_ioctl_screen, SCREEN_SETCURSOR, row, col, 0, 0, 0);
}

int z_draw_pixel(int x, int y, uint32_t color)
{
    return z_syscall(SYS_draw_pixel, x, y, color, 0, 0, 0);
}

int z_getcmdargs(char *buf, int size)
{
    return z_syscall(250, (long)buf, size, 0, 0, 0, 0);
}

#endif /* !KERNEL_SPACE */