#ifndef Z_SYSCALLS_H
#define Z_SYSCALLS_H

// Kernel space - use kernel types
typedef unsigned int uint32_t;
typedef int int32_t;
typedef unsigned long size_t;
typedef long ssize_t;
typedef long off_t;

// Protections (from sys/mman.h)
#define PROT_READ    0x1
#define PROT_WRITE   0x2
#define PROT_EXEC    0x4
#define PROT_NONE    0x0

// Flags (from sys/mman.h)
#define MAP_PRIVATE  0x02
#define MAP_ANONYMOUS 0x20
#define MAP_FIXED    0x10

// File open flags (from fcntl.h)
#define O_RDONLY     0

// Seek flags (from unistd.h)
#define SEEK_SET     0
#define SEEK_CUR     1
#define SEEK_END     2

// AT constants
#define AT_FDCWD     -100

#define z_errno	(*z_perrno())

int	z_exit(int status);
int	z_open(const char *pathname, int flags);
int	z_openat(int dirfd, const char *pathname, int flags);
int	z_close(int fd);
int	z_lseek(int fd, off_t offset, int whence);
ssize_t	z_read(int fd, void *buf, size_t count);
ssize_t	z_write(int fd, const void *buf, size_t count);
void	*z_mmap(void *addr, size_t length, int prot,
		int flags, int fd, off_t offset);
int	z_munmap(void *addr, size_t length);
int	z_mprotect(void *addr, size_t length, int prot);
int	*z_perrno(void);

#endif /* Z_SYSCALLS_H */
