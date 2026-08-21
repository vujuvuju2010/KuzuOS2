// syscall_linux_compat.h - Linux-compatible syscall definitions for KuzuOS5
#ifndef SYSCALL_LINUX_COMPAT_H
#define SYSCALL_LINUX_COMPAT_H

// ====== File I/O Syscalls ======
// Already defined: SYS_READ (3), SYS_WRITE (4), SYS_OPEN (5), SYS_CLOSE (6)
// SYS_LSEEK (19), SYS_OPENAT, SYS_READDIR (89), SYS_IOCTL (54)

#define SYS_DUP         41
#define SYS_DUP2        63
#define SYS_DUP3        330
#define SYS_FCNTL       55
#define SYS_FCNTL64     221
#define SYS_PIPE        42
#define SYS_PIPE2       328
#define SYS_POLL        168
#define SYS_PPOLL       309
#define SYS_SELECT      82
#define SYS_PSELECT6    308
#define SYS_STAT        18
#define SYS_STAT64      195
#define SYS_LSTAT       84
#define SYS_LSTAT64     196
#define SYS_FSTAT       28
#define SYS_FSTAT64     197
#define SYS_STATFS      99
#define SYS_FSTATFS     100
#define SYS_STATX       332

// ====== File Operations ======
#define SYS_UNLINK      10
#define SYS_UNLINKAT    310
#define SYS_RENAME      38
#define SYS_RENAMEAT    310
#define SYS_LINK        9
#define SYS_LINKAT      311
#define SYS_SYMLINK     83
#define SYS_SYMLINKAT   312
#define SYS_READLINK    85
#define SYS_READLINKAT  313
#define SYS_CHMOD       15
#define SYS_FCHMOD      94
#define SYS_FCHMODAT    314
#define SYS_CHOWN       182
#define SYS_FCHOWN      95
#define SYS_LCHOWN      16
#define SYS_FCHOWNAT    315
#define SYS_TRUNCATE    92
#define SYS_FTRUNCATE   93
#define SYS_FALLOCATE   324

// ====== Directory Operations ======
// Already defined: SYS_MKDIR (39), SYS_RMDIR (40), SYS_CHDIR (12)
#define SYS_FCHDIR      133
#define SYS_GETCWD      183
#define SYS_OPENDIR     228
#define SYS_READDIR     229
#define SYS_CLOSEDIR    230
#define SYS_REWINDDIR   231
#define SYS_TELLDIR     232
#define SYS_SEEKDIR     233

// ====== Process Management ======
// Already defined: SYS_EXIT (1), SYS_FORK (2), SYS_WAITPID (7)
#define SYS_CLONE       120
#define SYS_CLONE3      435
#define SYS_EXECVE      11
#define SYS_EXECVEAT    328
#define SYS_EXIT_GROUP  252
#define SYS_GETPID      20
#define SYS_GETPPID     64
#define SYS_GETPGID     132
#define SYS_SETPGID     57
#define SYS_GETPGRP     65
#define SYS_SETPGRP     66
#define SYS_GETSID      147
#define SYS_SETSID      66
#define SYS_PRCTL       172
#define SYS_ARCH_PRCTL  158

// ====== Signal Handling ======
#define SYS_SIGNAL      48
#define SYS_SIGACTION   67
#define SYS_SIGPROCMASK 126
#define SYS_SIGPENDING  73
#define SYS_SIGWAIT     128
#define SYS_SIGALTSTACK 186
#define SYS_KILL        37
#define SYS_TKILL       268
#define SYS_TGKILL      268
#define SYS_PAUSE       29
#define SYS_RT_SIGACTION 174
#define SYS_RT_SIGPROCMASK 175
#define SYS_RT_SIGPENDING  176
#define SYS_RT_SIGTIMEDWAIT 177
#define SYS_RT_SIGQUEUEINFO 178
#define SYS_RT_SIGSUSPEND  179

// ====== Memory Management ======
// Already defined: SYS_MMAP (90), SYS_MUNMAP (91), SYS_MPROTECT (93)
#define SYS_BRK         45
#define SYS_MREMAP      163
#define SYS_MSYNC       144
#define SYS_MADVISE     219
#define SYS_MLOCK       150
#define SYS_MUNLOCK     151
#define SYS_MLOCKALL    152
#define SYS_MUNLOCKALL  153

// ====== User/Group Management ======
// Already defined: SYS_SETUID (23), SYS_GETUID (24), SYS_SETGID (46), SYS_GETGID (47)
#define SYS_GETEUID     49
#define SYS_GETEGID     50
#define SYS_SETREUID    70
#define SYS_SETREGID    71
#define SYS_GETGROUPS   80
#define SYS_SETGROUPS   81
#define SYS_SETRESUID   164
#define SYS_GETRESUID   165
#define SYS_SETRESGID   170
#define SYS_GETRESGID   171

// ====== Time ======
#define SYS_TIME        13
#define SYS_STIME       25
#define SYS_GETTIMEOFDAY 78
#define SYS_SETTIMEOFDAY 79
#define SYS_CLOCK_GETTIME 263
#define SYS_CLOCK_SETTIME 264
#define SYS_CLOCK_NANOSLEEP 265
#define SYS_CLOCK_GETRES 266
#define SYS_NANOSLEEP   162
#define SYS_TIMER_CREATE 259
#define SYS_TIMER_SETTIME 260
#define SYS_TIMER_GETTIME 261
#define SYS_TIMER_GETOVERRUN 262
#define SYS_TIMER_DELETE 263
#define SYS_ALARM       27
#define SYS_GETITIMER   105
#define SYS_SETITIMER   104

// ====== Terminal Control ======
#define SYS_IOCTL       54
#define SYS_TCGETATTR   0x5401  // TCGETS ioctl
#define SYS_TCSETATTR   0x5402  // TCSETS ioctl

// ====== Process Tracing ======
#define SYS_PTRACE      26

// ====== IPC ======
#define SYS_MSGGET      236
#define SYS_MSGSND      234
#define SYS_MSGRCV      235
#define SYS_MSGCTL      237
#define SYS_SEMGET      238
#define SYS_SEMOP       239
#define SYS_SEMCTL      240
#define SYS_SHMGET      241
#define SYS_SHMAT       242
#define SYS_SHMDT       243
#define SYS_SHMCTL      244

// ====== Socket ======
#define SYS_SOCKET      281
#define SYS_BIND        282
#define SYS_CONNECT     283
#define SYS_LISTEN      284
#define SYS_ACCEPT      285
#define SYS_ACCEPT4     334
#define SYS_GETSOCKNAME 286
#define SYS_GETPEERNAME 287
#define SYS_SOCKETPAIR  288
#define SYS_SEND        289
#define SYS_RECV        292
#define SYS_SENDTO      290
#define SYS_RECVFROM    291
#define SYS_SHUTDOWN    293
#define SYS_SETSOCKOPT  294
#define SYS_GETSOCKOPT  295
#define SYS_SENDMSG     296
#define SYS_RECVMSG     297

// ====== Filesystem ======
#define SYS_MOUNT       21
#define SYS_UMOUNT      22
#define SYS_UMOUNT2     52
#define SYS_SYNC        36
#define SYS_FSYNC       118
#define SYS_FDATASYNC   119
#define SYS_SYNCFS      306

// ====== Misc ======
#define SYS_SLEEP       162
#define SYS_UNAME       122
#define SYS_UTIME       30
#define SYS_UTIMES      235
#define SYS_FUTIMESAT   299
#define SYS_GETPRIORITY 96
#define SYS_SETPRIORITY 97
#define SYS_RLIMIT      76
#define SYS_GETRLIMIT   191
#define SYS_SETRLIMIT   75
#define SYS_GETRUSAGE   77
#define SYS_NICE        34
#define SYS_SCHED_YIELD 158

#endif /* SYSCALL_LINUX_COMPAT_H */
