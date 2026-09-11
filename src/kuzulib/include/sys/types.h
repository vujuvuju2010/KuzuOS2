/* KuzuOS C Library - sys/types.h for Tor compatibility */
#ifndef _SYS_TYPES_H
#define _SYS_TYPES_H

#include <stdint.h>
#include <stddef.h>

/* Standard integer types */
typedef signed char         int8_t;
typedef unsigned char       uint8_t;
typedef signed short        int16_t;
typedef unsigned short      uint16_t;
typedef signed int          int32_t;
typedef unsigned int        uint32_t;
typedef signed long long    int64_t;
typedef unsigned long long  uint64_t;

/* Size types */
typedef unsigned long size_t;
typedef long ssize_t;
typedef long intptr_t;
typedef unsigned long uintptr_t;

/* File offset */
typedef long off_t;
typedef long long off64_t;

/* Socket length type */
typedef unsigned int socklen_t;

/* Poll nfds type */
typedef unsigned long nfds_t;

/* Process ID */
typedef int pid_t;
typedef unsigned int uid_t;
typedef unsigned int gid_t;
typedef int sid_t;

/* Time types */
typedef long time_t;
typedef long suseconds_t;
typedef long clock_t;

/* File descriptor */
typedef int fd_t;

/* Device types */
typedef unsigned int dev_t;
typedef unsigned long ino_t;
typedef unsigned short mode_t;
typedef unsigned short nlink_t;
typedef long blkcnt_t;
typedef long blksize_t;

/* Network types */
typedef uint16_t sa_family_t;
typedef uint32_t in_addr_t;
typedef uint32_t in_port_t;

/* NULL pointer */
#ifndef NULL
#define NULL ((void*)0)
#endif

#endif /* _SYS_TYPES_H */
