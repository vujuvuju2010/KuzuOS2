#ifndef _FCNTL_H
#define _FCNTL_H

#define O_RDONLY       0
#define O_WRONLY       1
#define O_RDWR         2
#define O_APPEND       0x0008
#define O_CREAT        0x0200
#define O_TRUNC        0x0400
#define O_EXCL         0x0800
#define O_SYNC         0x2000
#define O_NONBLOCK     0x0004
#define O_NOCTTY       0x0100

int open(const char *, int, ...);
int creat(const char *, int);
int fcntl(int, int, ...);

#endif
