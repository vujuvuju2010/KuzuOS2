#ifndef _SYS_TYPES_H
#define _SYS_TYPES_H

#include <stddef.h>
#include <stdint.h>

typedef int pid_t;

#ifndef _UID_T_DEFINED
#define _UID_T_DEFINED
typedef int uid_t;
typedef int gid_t;
#endif
typedef long off_t;
typedef int mode_t;
typedef int dev_t;
typedef int ino_t;
typedef int nlink_t;
typedef unsigned int blksize_t;
typedef unsigned long blkcnt_t;
typedef long time_t;
typedef long suseconds_t;
typedef int fd_set;

#endif
