/* KuzuOS C Library - types.h */
#ifndef _TYPES_H
#define _TYPES_H

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
typedef long          ssize_t;
typedef long          intptr_t;
typedef unsigned long uintptr_t;

/* File offset */
typedef long off_t;

/* Socket length type */
typedef unsigned int socklen_t;

/* Poll nfds type */
typedef unsigned long nfds_t;

/* Process ID */
typedef int pid_t;
typedef unsigned int uid_t;
typedef unsigned int gid_t;

/* Time types */
typedef long time_t;
typedef long suseconds_t;

/* File descriptor */
typedef int fd_t;

/* NULL pointer */
#ifndef NULL
#define NULL ((void*)0)
#endif

#endif
