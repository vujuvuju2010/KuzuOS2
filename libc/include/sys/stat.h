#ifndef _SYS_STAT_H
#define _SYS_STAT_H

#include <sys/types.h>
#include <time.h>

#define S_IFMT    0170000
#define S_IFREG   0100000
#define S_IFDIR   0040000
#define S_IFLNK   0120000
#define S_IFBLK   0060000
#define S_IFCHR   0020000
#define S_IFIFO   0010000
#define S_ISUID   0004000
#define S_ISGID   0002000
#define S_ISVTX   0001000

#define S_IRWXU  0700
#define S_IRUSR  0400
#define S_IWUSR  0200
#define S_IXUSR  0100
#define S_IRWXG  0070
#define S_IRGRP  0040
#define S_IWGRP  0020
#define S_IXGRP  0010
#define S_IRWXO  0007
#define S_IROTH  0004
#define S_IWOTH  0002
#define S_IXOTH  0001

struct stat {
    dev_t st_dev;
    ino_t st_ino;
    mode_t st_mode;
    nlink_t st_nlink;
    uid_t st_uid;
    gid_t st_gid;
    dev_t st_rdev;
    off_t st_size;
    time_t st_atime;
    time_t st_mtime;
    time_t st_ctime;
    blksize_t st_blksize;
    blkcnt_t st_blocks;
};

int stat(const char *path, struct stat *statbuf);
int fstat(int fd, struct stat *statbuf);
int lstat(const char *path, struct stat *statbuf);

#endif
