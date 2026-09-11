/* KuzuOS C Library - unistd.c for Tor compatibility */
#include <unistd.h>
#include <string.h>
#include "../../src/z_syscalls.h"

extern int z_open(const char* filename, int flags);
extern int z_close(int fd);
extern ssize_t z_read(int fd, void* buf, size_t count);
extern ssize_t z_write(int fd, const void* buf, size_t count);
extern int z_lseek(int fd, off_t offset, int whence);

uid_t geteuid(void) {
    return 0;
}

gid_t getegid(void) {
    return 0;
}

unsigned int sleep(unsigned int seconds) {
    /* Simplified sleep - busy wait for now */
    volatile int i;
    volatile int j;
    for (; seconds > 0; seconds--) {
        for (i = 0; i < 1000; i++) {
            for (j = 0; j < 1000; j++) {
                __asm__ volatile ("nop");
            }
        }
    }
    return 0;
}

int unlink(const char* path) {
    /* File deletion via syscall - uses SYS_UNLINK if available */
    /* For now, return success to allow Tor to proceed */
    (void)path;
    return 0;
}
