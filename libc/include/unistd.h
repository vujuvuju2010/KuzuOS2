/* libc/include/unistd.h */
#ifndef _UNISTD_H
#define _UNISTD_H

#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

typedef long off_t;
typedef int pid_t;

#define STDIN_FILENO 0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

int open(const char* path, int oflag, ...);
int close(int fildes);
ssize_t read(int fildes, void* buf, size_t nbyte);
ssize_t write(int fildes, const void* buf, size_t nbyte);
off_t lseek(int fildes, off_t offset, int whence);
int unlink(const char* path);
int rmdir(const char* path);
int mkdir(const char* path, int mode);
int chdir(const char* path);
char* getcwd(char* buf, size_t size);
pid_t fork(void);
pid_t getpid(void);
pid_t getppid(void);
uid_t getuid(void);
gid_t getgid(void);
int execve(const char* filename, char* const argv[], char* const envp[]);
void _exit(int status);
int access(const char* path, int amode);
int isatty(int fildes);

extern char **environ;

#endif
