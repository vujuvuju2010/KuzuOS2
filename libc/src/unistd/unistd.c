/* libc/src/unistd/unistd.c */
#include <unistd.h>
#include <string.h>
#include "../../src/z_syscalls.h"
extern int z_open(const char* filename, int flags);
extern int z_close(int fd);
extern ssize_t z_read(int fd, void* buf, size_t count);
extern ssize_t z_write(int fd, const void* buf, size_t count);
extern int z_lseek(int fd, off_t offset, int whence);

int open(const char* path, int oflag, ...) {
    return z_open(path, oflag);
}

int close(int fildes) {
    return z_close(fildes);
}

ssize_t read(int fildes, void* buf, size_t nbyte) {
    return z_read(fildes, buf, nbyte);
}

ssize_t write(int fildes, const void* buf, size_t nbyte) {
    return z_write(fildes, buf, nbyte);
}

off_t lseek(int fildes, off_t offset, int whence) {
    return (off_t)z_lseek(fildes, offset, whence);
}

int unlink(const char* path) {
    /* Not directly supported - would need syscall */
    return -1;
}

int rmdir(const char* path) {
    /* Not directly supported - would need syscall */
    return -1;
}

int mkdir(const char* path, int mode) {
    /* Wrap SYS_MKDIR via syscall, needs z_syscall wrapper */
    /* For now stub */
    return -1;
}

int chdir(const char* path) {
    /* Not directly supported - would need syscall */
    return -1;
}

char* getcwd(char* buf, size_t size) {
    /* Return / as current directory */
    if (buf && size > 1) {
        buf[0] = '/';
        buf[1] = '\0';
        return buf;
    }
    return NULL;
}

pid_t fork(void) {
    /* Not supported on KuzuOS5 */
    return -1;
}

pid_t getpid(void) {
    return 1;
}

pid_t getppid(void) {
    return 1;
}

uid_t getuid(void) {
    return 0;
}

gid_t getgid(void) {
    return 0;
}

int execve(const char* filename, char* const argv[], char* const envp[]) {
    /* Not supported */
    return -1;
}

int access(const char* path, int amode) {
    /* Simplified - assume file exists */
    return 0;
}

int isatty(int fildes) {
    /* Return true for stdout/stderr, false for files */
    return (fildes == 1 || fildes == 2) ? 1 : 0;
}

void _exit(int status) {
    extern void exit(int);
    exit(status);
}
