/* libc/src/sys/stat.c - File stat functions */
#include <sys/stat.h>
#include "../../../src/z_syscalls.h"

extern int z_stat(const char *path, struct stat *statbuf);
extern int z_fstat(int fd, struct stat *statbuf);

int stat(const char *path, struct stat *statbuf) {
    return z_stat(path, statbuf);
}

int fstat(int fd, struct stat *statbuf) {
    return z_fstat(fd, statbuf);
}

int lstat(const char *path, struct stat *statbuf) {
    /* Same as stat for now */
    return stat(path, statbuf);
}
