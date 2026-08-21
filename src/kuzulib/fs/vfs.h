/*za headers for vfs.c and a lotttt more*/
#ifndef VFS_H
#define VFS_H

typedef unsigned char  uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int   uint32_t;

#define VFS_PATH_MAX 256

/* open flags */
#define VFS_FLAG_READ  0x01
#define VFS_FLAG_WRITE 0x02
#define VFS_FLAG_DIR   0x04

/* fs type tags */
#define VFS_FS_RAMFS 0
#define VFS_FS_FAT32 1

/* forward decls */
struct vfs_node;
struct mount_entry;

/* vtable every backend must fill */
typedef struct fs_driver {
    int (*open)   (struct vfs_node *node, const char *path, int flags);
    int (*read)   (struct vfs_node *node, void *buf, uint32_t len);
    int (*write)  (struct vfs_node *node, const void *buf, uint32_t len);
    int (*close)  (struct vfs_node *node);
    int (*readdir)(struct vfs_node *node, uint32_t index,
                   char *name_out, int *is_dir_out);
    int (*stat)   (struct mount_entry *mnt, const char *path,
                   uint32_t *size_out, int *is_dir_out);
} fs_driver_t;

/* one row in the mount table */
typedef struct mount_entry {
    char         mountpoint[VFS_PATH_MAX];
    fs_driver_t *driver;
    void        *fs_data;
    int          used;
} mount_entry_t;

/* one open file descriptor */
typedef struct vfs_node {
    mount_entry_t *mount;
    char           path[VFS_PATH_MAX];
    int            flags;
    uint32_t       offset;
    void          *fs_data;
    int            used;
} vfs_node_t;

/* ── core VFS api ───────────────────────────────────────────── */
void vfs_init    (void);
int  vfs_mount   (const char *mountpoint, int fs_type, void *fs_private);
int  vfs_unmount (const char *mountpoint);
int  vfs_open    (vfs_node_t **fds, const char *path, int flags);
int  vfs_read    (vfs_node_t **fds, int fd, void *buf, uint32_t len);
int  vfs_write   (vfs_node_t **fds, int fd, const void *buf, uint32_t len);
int  vfs_close   (vfs_node_t **fds, int fd);
int  vfs_seek    (vfs_node_t **fds, int fd, int offset, int whence);
int  vfs_readdir (vfs_node_t **fds, int fd, uint32_t index,
                  char *name_out, int *is_dir_out);
int  vfs_stat    (const char *path, uint32_t *size_out, int *is_dir_out);
int  vfs_set_max_mounts(int new_max);

/* ── ring 3 syscall handlers ────────────────────────────────── */
/*
 * fd 0 = stdin  → console_read  (handled inside sys_read)
 * fd 1 = stdout → console_write (handled inside sys_write)
 * fd 2 = stderr → console_write (handled inside sys_write)
 * fd 3+ = vfs_node_t* in current_process->fds[]
 */
int  sys_open    (const char *path, int flags);
int  sys_read    (int fd, void *buf, uint32_t len);
int  sys_write   (int fd, const void *buf, uint32_t len);
int  sys_close   (int fd);
int  sys_lseek   (int fd, int offset, int whence);
int  sys_readdir (int fd, uint32_t index, char *name_out, int *is_dir_out);
int  sys_stat    (const char *path, uint32_t *size_out, int *is_dir_out);
void sys_close_all(void);   /* close all fds for current_process — call on exit */
int vfs_strcmp(const char *a, const char *b); /* also used in fs_chdir */
#endif