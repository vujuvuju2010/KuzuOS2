/*VFS FOR KUZUOS2
WRITTEN BY KUZEY AND CLAUDE*/
// so okay normally it was half me half claude but for the sake of the syscalls and my time i leave it to all claude so have fun 


#include "vfs.h"
#include "../../memory.h"
#include "../../process.h"
#include "../../fatfs/ff.h"
#include "../../filesystem.h"

// Forward declare ramfs_entry_t structure
typedef struct {
    char path[256];
    char name[64];
    int is_directory;
    char *data;
    uint32_t size;
    int used;
} ramfs_entry_t;

static int maxprocess = 16; // changeable via vfs_set_max_mounts syscall
static int maxfds = 64;

static mount_entry_t *mount_table = 0;

/* ═══════════════════════════════════════════════════════════════
   Internal string helpers (no libc dependency)
   ═══════════════════════════════════════════════════════════════ */

static int vfs_strlen(const char *s){
    int n = 0;
    while(s[n]) n++;
    return n;
}

int vfs_strcmp(const char *a, const char *b){ // cant be static since used in filesystem.c fs_chdir for cd 
    while(*a && *b){
        if(*a != *b) return *a - *b;
        a++; b++;
    }
    return *a - *b;
}

static int vfs_strncmp(const char *a, const char *b, int n){
    for(int i = 0; i < n; i++){
        if(a[i] != b[i]) return a[i] - b[i];
        if(a[i] == 0)    return 0;
    }
    return 0;
}

static void vfs_strcpy(char *dst, const char *src){
    while((*dst++ = *src++));
}

static void vfs_memset(void *p, int c, uint32_t n){
    uint8_t *d = (uint8_t *)p;
    while(n--) *d++ = (uint8_t)c;
}

/* ═══════════════════════════════════════════════════════════════
   Mount table helpers
   ═══════════════════════════════════════════════════════════════ */

mount_entry_t *find_mount(const char *path){
    mount_entry_t *best = 0;
    int best_len = -1;
    for(int i = 0; i < maxprocess; i++){
        if(!mount_table[i].used) continue;
        int mlen = vfs_strlen(mount_table[i].mountpoint);
        if(vfs_strncmp(path, mount_table[i].mountpoint, mlen) == 0){
            char next = path[mlen];
            // root mount '/' matches everything
            // other mounts only match if followed by '/' or end of string
            int matches = (mlen == 1) ? 1 : (next == '\0' || next == '/');
            if(matches && mlen > best_len){
                best_len = mlen;
                best = &mount_table[i];
            }
        }
    }
    return best;
}

static const char *relative_path(mount_entry_t *mnt, const char *path){
    int mlen = vfs_strlen(mnt->mountpoint);
    if(mlen == 1 && mnt->mountpoint[0] == '/') return path;
    const char *rel = path + mlen;
    if(*rel == '\0') return "/";
    return rel;
}

static int alloc_fd(vfs_node_t **fds){
    /* fd 0/1/2 are reserved for stdin/stdout/stderr */
    for(int i = 3; i < maxfds; i++){
        if(!fds[i]) return i;
    }
    return -1;
}

/* ═══════════════════════════════════════════════════════════════
   Console I/O — called by sys_read/sys_write for fd 0/1/2
   ═══════════════════════════════════════════════════════════════ */

/* These live in your existing kernel — we just declare them */
extern void putchar_ansi(char c);
extern void keyboard_poll(void);
extern char keyboard_get_char(void);
extern void usb_poll(void);

static int console_write(const void *buf, uint32_t len){
    const char *s = (const char *)buf;
    for(uint32_t i = 0; i < len; i++) putchar_ansi(s[i]);
    return (int)len;
}

static int console_read(void *buf, uint32_t len){
    char *dst = (char *)buf;
    uint32_t i = 0;
    while(i < len){
        char c = 0;
        while(c == 0){
            keyboard_poll();
            usb_poll();
            c = keyboard_get_char();
            if(c == 0){
                for(volatile int d = 0; d < 1000; d++);
            }
        }
        /* backspace handling */
        if(c == '\b' || c == 127){
            if(i > 0){
                i--;
                putchar_ansi('\b');
                putchar_ansi(' ');
                putchar_ansi('\b');
            }
            continue;
        }
        putchar_ansi(c); /* echo */
        dst[i++] = c;
        if(c == '\n' || c == '\r'){
            dst[i - 1] = '\n';
            break;
        }
    }
    return (int)i;
}

/* ═══════════════════════════════════════════════════════════════
   RAMFS backend
   ═══════════════════════════════════════════════════════════════ */

static int ramfs_open(struct vfs_node *node, const char *path, int flags){
    extern int ramfs_exists(const char *path);
    
    // Check in-memory ramfs first
    if (ramfs_exists(path)) {
        node->offset = 0;
        node->fs_data = 0;
        return 0;
    }
    
    // Fall back to disk filesystem
    if(!fs_any_exists((char *)path)) return -1;
    node->offset = 0;
    node->fs_data = 0;
    return 0;
}

static int ramfs_read(struct vfs_node *node, void *buf, uint32_t len){
    if(node->flags & VFS_FLAG_DIR) return -1;
    int fsize = fs_get_file_size((char *)node->path);
    if(fsize < 0) return -1;
    if(node->offset >= (uint32_t)fsize) return 0;

    uint32_t remaining = (uint32_t)fsize - node->offset;
    if(len > remaining) len = remaining;

    char *tmp = (char *)kmalloc((uint32_t)fsize);
    if(!tmp) return -1;

    int got = fs_read_file((char *)node->path, tmp, (uint32_t)fsize);
    if(got < 0){ kfree(tmp); return -1; }

    uint8_t *dst = (uint8_t *)buf;
    uint8_t *src = (uint8_t *)tmp + node->offset;
    for(uint32_t i = 0; i < len; i++) dst[i] = src[i];

    node->offset += len;
    kfree(tmp);
    return (int)len;
}

static int ramfs_write(struct vfs_node *node, const void *buf, uint32_t len){
    if(node->flags & VFS_FLAG_DIR) return -1;
    int r = fs_write_file((char *)node->path, (char *)buf, len);
    if(r != 0) return -1;
    node->offset += len;
    return (int)len;
}

static int ramfs_close(struct vfs_node *node){
    (void)node;
    return 0;
}

static int ramfs_readdir(struct vfs_node *node, uint32_t index,
                          char *name_out, int *is_dir_out){
    extern int ramfs_get_entry_by_index_in_dir(const char *dir_path, uint32_t index, char **name, int *is_dir);
    
    // First, enumerate in-memory ramfs entries that are children of this directory
    char *ramfs_name;
    int ramfs_isdir;
    if (ramfs_get_entry_by_index_in_dir(node->path, index, &ramfs_name, &ramfs_isdir) == 0) {
        // Copy name
        int i = 0;
        while (ramfs_name[i] && i < 255) {
            name_out[i] = ramfs_name[i];
            i++;
        }
        name_out[i] = '\0';
        *is_dir_out = ramfs_isdir;
        return 1;
    }
    
    // Fall back to disk filesystem
    return fs_readdir_index((char *)node->path, index, name_out, is_dir_out);
}

static int ramfs_stat(struct mount_entry *mnt, const char *path,
                       uint32_t *size_out, int *is_dir_out){
    (void)mnt;
    
    extern int ramfs_exists(const char *path);
    extern ramfs_entry_t* ramfs_get_entry(const char *path);
    
    // Check virtual directories first
    extern virtual_mount_t* find_virtual_mount(const char *path);
    virtual_mount_t *vmount = find_virtual_mount(path);
    if (vmount && vmount->stat_virtual) {
        int result = vmount->stat_virtual(path, size_out, is_dir_out);
        if (result == 0) return 0;
    }
    
    // Check in-memory ramfs first
    if (ramfs_exists(path)) {
        ramfs_entry_t *entry = ramfs_get_entry(path);
        if (entry) {
            *is_dir_out = entry->is_directory;
            *size_out = entry->size;
            return 0;
        }
    }
    
    // Fall back to disk filesystem
    if(!fs_any_exists((char *)path)) return -1;
    *is_dir_out = fs_any_is_directory((char *)path);
    if(*is_dir_out){
        *size_out = 0;
    } else {
        int sz = fs_get_file_size((char *)path);
        *size_out = (sz < 0) ? 0 : (uint32_t)sz;
    }
    return 0;
}

static fs_driver_t ramfs_driver = {
    ramfs_open,
    ramfs_read,
    ramfs_write,
    ramfs_close,
    ramfs_readdir,
    ramfs_stat,
};

/* ═══════════════════════════════════════════════════════════════
   FAT32 backend
   ═══════════════════════════════════════════════════════════════ */

static int fat32_open(struct vfs_node *node, const char *path, int flags){
    extern void print(const char *); extern void print_color(const char *, int);
    struct mount_entry *mnt = node->mount;
    const char *rel = relative_path((mount_entry_t *)mnt, path);

    char fatpath[VFS_PATH_MAX + 3];
    fatpath[0] = '1'; fatpath[1] = ':';
    int i = 2;
    
    /* Special case: root directory - FatFS wants "1:" not "1:/" for opendir */
    if(rel[0] == '/' && rel[1] == '\0'){
        if(flags & VFS_FLAG_DIR){
            fatpath[2] = '\0';  /* "1:" for root */
            print_color("[fat32_open] ROOT DIR fatpath='", 0x0E);
            print(fatpath); print("'\n");
        } else {
            /* Can't open root as file */
            print_color("[fat32_open] Can't open root as file\n", 0x0C);
            return -1;
        }
    } else {
        // Skip leading slash for non-root paths
        if (*rel == '/') rel++;
        while(*rel && i < VFS_PATH_MAX + 2) fatpath[i++] = *rel++;
        fatpath[i] = '\0';
    }
    
    print_color("[fat32_open] path='", 0x0A); print(path);
    print("' fatpath='"); print(fatpath);
    print("' flags=0x"); 
    char hbuf[8]; int p=0; unsigned int f=(unsigned int)flags;
    for(int shift=28; shift>=0; shift-=4){
        int nib=(f>>shift)&0xF;
        hbuf[p++]="0123456789ABCDEF"[nib];
    }
    hbuf[p]=0; print(hbuf); print("\n");

    if(flags & VFS_FLAG_DIR){
        DIR *dp = (DIR *)kmalloc(sizeof(DIR));
        if(!dp){ print_color("[fat32_open] kmalloc failed\n", 0x0C); return -1; }
        vfs_memset(dp, 0, sizeof(DIR));
        FRESULT res = f_opendir(dp, fatpath);
        if(res != FR_OK){ 
            print_color("[fat32_open] f_opendir FAILED code=", 0x0C);
            char buf[8]; int bp=0; int r=(int)res;
            if(r==0) buf[bp++]='0'; else { char tmp[8]; int t=0;
            while(r>0){ tmp[t++]='0'+(r%10); r/=10; }
            while(t-->0) buf[bp++]=tmp[t]; }
            buf[bp]=0; print(buf); print("\n");
            kfree(dp); return -1; 
        }
        print_color("[fat32_open] f_opendir OK\n", 0x0A);
        node->fs_data = dp;
    } else {
        FIL *fp = (FIL *)kmalloc(sizeof(FIL));
        if(!fp) return -1;
        vfs_memset(fp, 0, sizeof(FIL));
        uint8_t mode = 0;
        if(flags & VFS_FLAG_READ)  mode |= FA_READ;
        if(flags & VFS_FLAG_WRITE) mode |= FA_WRITE | FA_OPEN_ALWAYS;
        if(f_open(fp, fatpath, mode) != FR_OK){ kfree(fp); return -1; }
        node->fs_data = fp;
    }
    node->offset = 0;
    return 0;
}

static int fat32_read(struct vfs_node *node, void *buf, uint32_t len){
    FIL *fp = (FIL *)node->fs_data;
    if(!fp) return -1;
    uint32_t br = 0;
    if(f_read(fp, buf, len, &br) != FR_OK) return -1;
    node->offset += br;
    return (int)br;
}

static int fat32_write(struct vfs_node *node, const void *buf, uint32_t len){
    FIL *fp = (FIL *)node->fs_data;
    if(!fp) return -1;
    uint32_t bw = 0;
    if(f_write(fp, buf, len, &bw) != FR_OK) return -1;
    node->offset += bw;
    return (int)bw;
}

static int fat32_close(struct vfs_node *node){
    if(!node->fs_data) return 0;
    if(node->flags & VFS_FLAG_DIR)
        f_closedir((DIR *)node->fs_data);
    else
        f_close((FIL *)node->fs_data);
    kfree(node->fs_data);
    node->fs_data = 0;
    return 0;
}

static int fat32_readdir(struct vfs_node *node, uint32_t index,
                          char *name_out, int *is_dir_out){
    extern void print(const char *); extern void print_color(const char *, int);

    struct mount_entry *mnt = (struct mount_entry *)node->mount;
    const char *rel = relative_path((mount_entry_t *)mnt, node->path);

    char fatpath[VFS_PATH_MAX + 3];
    fatpath[0] = '0'; fatpath[1] = ':';
    int i = 2;
    if(rel[0] == '/' && rel[1] == '\0'){
        fatpath[2] = '\0'; // must be 0: for root
    } else {
        // Skip leading slash if present
        if (*rel == '/') rel++;
        while(*rel && i < VFS_PATH_MAX + 2) fatpath[i++] = *rel++;
        fatpath[i] = '\0';
    }

    /* Re-open fresh every time — f_rewinddir is unreliable on root */
    DIR tmp_dir;
    vfs_memset(&tmp_dir, 0, sizeof(DIR));
    FRESULT ro = f_opendir(&tmp_dir, fatpath);
    if(ro != FR_OK){
        print_color("[fat32_readdir] f_opendir failed code=", 0x0C);
        char buf[4]; int bp=0; int r=(int)ro;
        if(r==0) buf[bp++]='0'; else { char tmp[4]; int t=0;
            while(r>0){ tmp[t++]='0'+(r%10); r/=10; }
            while(t-->0) buf[bp++]=tmp[t]; }
        buf[bp]=0; print(buf); print("\n");
        return -1;
    }

    FILINFO fno;
    int result = 0;
    for(uint32_t j = 0; j <= index; j++){
        FRESULT res = f_readdir(&tmp_dir, &fno);
        if(res != FR_OK || fno.fname[0] == '\0'){
            result = 0;
            goto done;
        }
        if(j == index){
            int k = 0;
            while(fno.fname[k] && k < 255){ name_out[k] = fno.fname[k]; k++; }
            name_out[k] = '\0';
            *is_dir_out = (fno.fattrib & AM_DIR) ? 1 : 0;
            print_color("[fat32_readdir] FOUND '", 0x0A);
            print(name_out); print("'\n");
            result = 1;
        }
    }
done:
    f_closedir(&tmp_dir);
    return result;
}

static int fat32_stat(struct mount_entry *mnt, const char *path,
                       uint32_t *size_out, int *is_dir_out){
    extern void print(const char *); extern void print_color(const char *, int);
    const char *rel = relative_path((mount_entry_t *)mnt, path);
    char fatpath[VFS_PATH_MAX + 3];
    fatpath[0] = '1'; fatpath[1] = ':';
    int i = 2;
    
    /* Special case: root directory - FatFS wants "1:" not "1:/" */
    if(rel[0] == '/' && rel[1] == '\0'){
        fatpath[2] = '/'; fatpath[3] = '\0';   
        print_color("[fat32_stat] ROOT DIR path='", 0x0E);
        print(path); print("' fatpath='"); print(fatpath); print("'\n");
        /* Root directory is always a directory */
        *is_dir_out = 1;
        *size_out = 0;
        return 0;
    }
    
    // Skip leading slash for non-root paths
    if (*rel == '/') rel++;
    while(*rel && i < VFS_PATH_MAX + 2) fatpath[i++] = *rel++;
    fatpath[i] = '\0';
    
    print_color("[fat32_stat] path='", 0x0A); print(path);
    print("' rel='"); print(rel); print("' fatpath='"); print(fatpath); print("'\n");
    
    FILINFO fno;
    FRESULT res = f_stat(fatpath, &fno);
    if(res != FR_OK){
        print_color("[fat32_stat] FAILED code=", 0x0C);
        char buf[8]; int p=0; int r=(int)res;
        if(r==0) buf[p++]='0'; else { char tmp[8]; int t=0;
        while(r>0){ tmp[t++]='0'+(r%10); r/=10; }
        while(t-->0) buf[p++]=tmp[t]; }
        buf[p]=0; print(buf); print("\n");
        return -1;
    }
    *is_dir_out = (fno.fattrib & AM_DIR) ? 1 : 0;
    *size_out   = (uint32_t)fno.fsize;
    print_color("[fat32_stat] OK isdir=", 0x0A);
    print(*is_dir_out ? "1\n" : "0\n");
    return 0;
}

static fs_driver_t fat32_driver = {
    fat32_open,
    fat32_read,
    fat32_write,
    fat32_close,
    fat32_readdir,
    fat32_stat,
};

/* ═══════════════════════════════════════════════════════════════
   VFS core
   ═══════════════════════════════════════════════════════════════ */

void vfs_init(void){
    mount_table = (mount_entry_t *)kmalloc(sizeof(mount_entry_t) * maxprocess);
    vfs_memset(mount_table, 0, sizeof(mount_entry_t) * maxprocess);
}

int vfs_set_max_mounts(int new_max){
    if(new_max <= maxprocess) return -1;
    mount_entry_t *new_table = (mount_entry_t *)kmalloc(sizeof(mount_entry_t) * new_max);
    if(!new_table) return -1;
    vfs_memset(new_table, 0, sizeof(mount_entry_t) * new_max);
    for(int i = 0; i < maxprocess; i++) new_table[i] = mount_table[i];
    kfree(mount_table);
    mount_table = new_table;
    maxprocess  = new_max;
    return 0;
}

int vfs_mount(const char *mountpoint, int fs_type, void *fs_private){
    int slot = -1;
    for(int i = 0; i < maxprocess; i++){
        if(!mount_table[i].used){ slot = i; break; }
    }
    if(slot == -1) return -1;

    vfs_strcpy(mount_table[slot].mountpoint, mountpoint);
    mount_table[slot].fs_data = fs_private;
    mount_table[slot].driver  = (fs_type == VFS_FS_FAT32) ? &fat32_driver : &ramfs_driver;
    mount_table[slot].used    = 1;
    return 0;
}

int vfs_unmount(const char *mountpoint){
    for(int i = 0; i < maxprocess; i++){
        if(mount_table[i].used &&
           vfs_strcmp(mount_table[i].mountpoint, mountpoint) == 0){
            vfs_memset(&mount_table[i], 0, sizeof(mount_entry_t));
            return 0;
        }
    }
    return -1;
}

/* ── vfs_open ────────────────────────────────────────────────── */

int vfs_open(vfs_node_t **fds, const char *path, int flags){
    mount_entry_t *mnt = find_mount(path);
    if(!mnt) return -1;

    int fd = alloc_fd(fds);
    if(fd < 0) return -1;

    vfs_node_t *node = (vfs_node_t *)kmalloc(sizeof(vfs_node_t));
    if(!node) return -1;
    vfs_memset(node, 0, sizeof(vfs_node_t));

    node->mount = (struct mount_entry *)mnt;
    node->flags = flags;
    vfs_strcpy(node->path, path);

    /* auto-detect dir if not explicitly told */
    if(!(flags & VFS_FLAG_DIR)){
        uint32_t sz; int isdir = 0;
        if(mnt->driver->stat((struct mount_entry *)mnt, path, &sz, &isdir) == 0 && isdir)
            node->flags |= VFS_FLAG_DIR;
    }

    if(mnt->driver->open(node, path, node->flags) != 0){
        kfree(node);
        return -1;
    }

    fds[fd] = node;
    return fd;
}

/* ── vfs_read ────────────────────────────────────────────────── */

int vfs_read(vfs_node_t **fds, int fd, void *buf, uint32_t len){
    if(fd < 0 || fd >= maxfds || !fds[fd]) return -1;
    return fds[fd]->mount->driver->read(fds[fd], buf, len);
}

/* ── vfs_write ───────────────────────────────────────────────── */

int vfs_write(vfs_node_t **fds, int fd, const void *buf, uint32_t len){
    if(fd < 0 || fd >= maxfds || !fds[fd]) return -1;
    return fds[fd]->mount->driver->write(fds[fd], buf, len);
}

/* ── vfs_close ───────────────────────────────────────────────── */

int vfs_close(vfs_node_t **fds, int fd){
    if(fd < 0 || fd >= maxfds || !fds[fd]) return -1;
    fds[fd]->mount->driver->close(fds[fd]);
    kfree(fds[fd]);
    fds[fd] = 0;
    return 0;
}

/* ── vfs_seek ────────────────────────────────────────────────── */
/* whence: 0=SEEK_SET, 1=SEEK_CUR, 2=SEEK_END */

int vfs_seek(vfs_node_t **fds, int fd, int offset, int whence){
    if(fd < 0 || fd >= maxfds || !fds[fd]) return -1;
    vfs_node_t *node = fds[fd];

    uint32_t file_size = 0;
    int isdir = 0;
    /* ask the driver for size so SEEK_END works */
    if(node->mount && node->mount->driver->stat)
        node->mount->driver->stat(node->mount, node->path, &file_size, &isdir);

    uint32_t new_offset;
    switch(whence){
        case 0: /* SEEK_SET */
            if(offset < 0) return -1;
            new_offset = (uint32_t)offset;
            break;
        case 1: /* SEEK_CUR */
            new_offset = node->offset + (uint32_t)offset;
            break;
        case 2: /* SEEK_END */
            new_offset = file_size + (uint32_t)offset;
            break;
        default:
            return -1;
    }

    if(new_offset > file_size) new_offset = file_size;
    node->offset = new_offset;

    /* for FAT32 we also need to tell FatFs to seek */
    if(node->fs_data && !(node->flags & VFS_FLAG_DIR)){
        FIL *fp = (FIL *)node->fs_data;
        f_lseek(fp, new_offset);
    }

    return (int)new_offset;
}

/* ── vfs_readdir ─────────────────────────────────────────────── */

int vfs_readdir(vfs_node_t **fds, int fd, uint32_t index,
                char *name_out, int *is_dir_out){
    if(fd < 0 || fd >= maxfds || !fds[fd]) return -1;
    return fds[fd]->mount->driver->readdir(fds[fd], index, name_out, is_dir_out);
}

/* ── vfs_stat ────────────────────────────────────────────────── */

int vfs_stat(const char *path, uint32_t *size_out, int *is_dir_out){
    mount_entry_t *mnt = find_mount(path);
    if(!mnt) return -1;
    return mnt->driver->stat((struct mount_entry *)mnt, path, size_out, is_dir_out);
}

/* ═══════════════════════════════════════════════════════════════
   Ring 3 syscall handlers
   fd 0 = stdin  (console read)
   fd 1 = stdout (console write)
   fd 2 = stderr (console write)
   fd 3+ = VFS nodes in current_process->fds
   ═══════════════════════════════════════════════════════════════ */

extern struct process *current_process;

static vfs_node_t **get_fds(void){
    if(!current_process) return 0;
    return current_process->fds;
}

int sys_open(const char *path, int flags){
    vfs_node_t **fds = get_fds();
    if(!fds) return -1;
    return vfs_open(fds, path, flags);
}

int sys_read(int fd, void *buf, uint32_t len){
    /* console stdin */
    if(fd == 0) return console_read(buf, len);

    vfs_node_t **fds = get_fds();
    if(!fds) return -1;
    return vfs_read(fds, fd, buf, len);
}

int sys_write(int fd, const void *buf, uint32_t len){
    /* console stdout / stderr */
    if(fd == 1 || fd == 2) return console_write(buf, len);

    vfs_node_t **fds = get_fds();
    if(!fds) return -1;
    return vfs_write(fds, fd, buf, len);
}

int sys_close(int fd){
    /* never close stdio */
    if(fd <= 2) return 0;

    vfs_node_t **fds = get_fds();
    if(!fds) return -1;
    return vfs_close(fds, fd);
}

int sys_lseek(int fd, int offset, int whence){
    if(fd <= 2) return -1; /* can't seek console */

    vfs_node_t **fds = get_fds();
    if(!fds) return -1;
    return vfs_seek(fds, fd, offset, whence);
}

int sys_readdir(int fd, uint32_t index, char *name_out, int *is_dir_out){
    vfs_node_t **fds = get_fds();
    if(!fds) return -1;
    return vfs_readdir(fds, fd, index, name_out, is_dir_out);
}

int sys_stat(const char *path, uint32_t *size_out, int *is_dir_out){
    return vfs_stat(path, size_out, is_dir_out);
}

/* ── vfs_mkdir ───────────────────────────────────────────── */

int vfs_mkdir(const char *path) {
    extern void print(const char *);
    extern char kernel_cwd[256];
    
    print("[vfs_mkdir] called for ");
    print(path);
    print("\n");
    
    // Resolve relative paths
    char resolved[VFS_PATH_MAX];
    if (path[0] != '/') {
        int i = 0;
        while (kernel_cwd[i] && i < VFS_PATH_MAX - 1) {
            resolved[i] = kernel_cwd[i];
            i++;
        }
        if (i > 1 && resolved[i-1] != '/') resolved[i++] = '/';
        int j = 0;
        while (path[j] && i < VFS_PATH_MAX - 1) {
            resolved[i++] = path[j++];
        }
        resolved[i] = '\0';
        path = resolved;
        
        print("[vfs_mkdir] resolved to: ");
        print(path);
        print("\n");
    }
    
    // Find which filesystem this path belongs to
    mount_entry_t *mnt = find_mount(path);
    if (!mnt) {
        print("[vfs_mkdir] no mount found for path\n");
        return -1;
    }
    
    // Check if this is FAT32 filesystem
    if (mnt->driver == &fat32_driver) {
        print("[vfs_mkdir] using FAT32 backend\n");
        
        // Convert VFS path to FAT32 path
        const char *rel = relative_path(mnt, path);
        char fatpath[VFS_PATH_MAX + 3];
        fatpath[0] = '0';
        fatpath[1] = ':';
        int i = 2;
        
        // Can't create root directory
        if (rel[0] == '/' && rel[1] == '\0') {
            print("[vfs_mkdir] cannot create root directory\n");
            return -1;
        }
        
        while(*rel && i < VFS_PATH_MAX + 2) {
            fatpath[i++] = *rel++;
        }
        fatpath[i] = '\0';
        
        print("[vfs_mkdir] FAT32 path: ");
        print(fatpath);
        print("\n");
        
        // Create directory using FatFS
        FRESULT res = f_mkdir(fatpath);
        if (res != FR_OK) {
            print("[vfs_mkdir] f_mkdir failed code=");
            char buf[8];
            int p = 0;
            int r = (int)res;
            if (r == 0) buf[p++] = '0';
            else {
                char tmp[8];
                int t = 0;
                while (r > 0) {
                    tmp[t++] = '0' + (r % 10);
                    r /= 10;
                }
                while (t-- > 0) buf[p++] = tmp[t];
            }
            buf[p] = 0;
            print(buf);
            print("\n");
            return -1;
        }
        
        print("[vfs_mkdir] f_mkdir OK\n");
        return 0;
    }
    
    // Use in-memory ramfs for other filesystems
    print("[vfs_mkdir] using RAMFS backend\n");
    extern int ramfs_create_directory(const char *path);
    int result = ramfs_create_directory(path);
    
    print("[vfs_mkdir] ramfs_create_directory returned ");
    if (result == 0) print("OK");
    else print("FAIL");
    print("\n");
    
    return result;
}

/* ── vfs_create ──────────────────────────────────────────────── */

int vfs_create(const char *path) {
    extern void print(const char *);
    extern char kernel_cwd[256];
    
    print("[vfs_create] called for ");
    print(path);
    print("\n");
    
    // Resolve relative paths
    char resolved[VFS_PATH_MAX];
    if (path[0] != '/') {
        int i = 0;
        while (kernel_cwd[i] && i < VFS_PATH_MAX - 1) {
            resolved[i] = kernel_cwd[i];
            i++;
        }
        if (i > 1 && resolved[i-1] != '/') resolved[i++] = '/';
        int j = 0;
        while (path[j] && i < VFS_PATH_MAX - 1) {
            resolved[i++] = path[j++];
        }
        resolved[i] = '\0';
        path = resolved;
        
        print("[vfs_create] resolved to: ");
        print(path);
        print("\n");
    }
    
    // Find which filesystem this path belongs to
    mount_entry_t *mnt = find_mount(path);
    if (!mnt) {
        print("[vfs_create] no mount found for path\n");
        return -1;
    }
    
    // Check if this is FAT32 filesystem
    if (mnt->driver == &fat32_driver) {
        print("[vfs_create] using FAT32 backend\n");
        
        // Convert VFS path to FAT32 path
        const char *rel = relative_path(mnt, path);
        char fatpath[VFS_PATH_MAX + 3];
        fatpath[0] = '0';
        fatpath[1] = ':';
        int i = 2;
        
        while(*rel && i < VFS_PATH_MAX + 2) {
            fatpath[i++] = *rel++;
        }
        fatpath[i] = '\0';
        
        print("[vfs_create] FAT32 path: ");
        print(fatpath);
        print("\n");
        
        // Create empty file using FatFS
        FIL fp;
        FRESULT res = f_open(&fp, fatpath, FA_CREATE_NEW | FA_WRITE);
        if (res != FR_OK) {
            print("[vfs_create] f_open failed code=");
            char buf[8];
            int p = 0;
            int r = (int)res;
            if (r == 0) buf[p++] = '0';
            else {
                char tmp[8];
                int t = 0;
                while (r > 0) {
                    tmp[t++] = '0' + (r % 10);
                    r /= 10;
                }
                while (t-- > 0) buf[p++] = tmp[t];
            }
            buf[p] = 0;
            print(buf);
            print("\n");
            return -1;
        }
        
        f_close(&fp);
        print("[vfs_create] file created OK\n");
        return 0;
    }
    
    // Use in-memory ramfs for other filesystems
    print("[vfs_create] using RAMFS backend\n");
    extern int ramfs_create_file(const char *path);
    return ramfs_create_file(path);
}

/* close every vfs node a process has open — call on process exit */
void sys_close_all(void){
    vfs_node_t **fds = get_fds();
    if(!fds) return;
    for(int i = 3; i < maxfds; i++){
        if(fds[i]) vfs_close(fds, i);
    }
}

/* alias for process.c */
void close_all_fds(void){
    sys_close_all();
}
