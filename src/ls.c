// LS FOR KUZUOS2

#define SYS_WRITE    4
#define SYS_OPEN     5
#define SYS_CLOSE    6
#define SYS_READDIR  141
#define SYS_EXIT     1
#define SYS_GETCWD   183

#define VFS_FLAG_READ  0x01
#define VFS_FLAG_DIR   0x04

static inline int syscall1(int num, int a1) {
    int ret;
    __asm__ volatile("int $0x80" : "=a"(ret) : "a"(num), "b"(a1));
    return ret;
}
static inline int syscall2(int num, int a1, int a2) {
    int ret;
    __asm__ volatile("int $0x80" : "=a"(ret) : "a"(num), "b"(a1), "c"(a2));
    return ret;
}
static inline int syscall3(int num, int a1, int a2, int a3) {
    int ret;
    __asm__ volatile("int $0x80" : "=a"(ret) : "a"(num), "b"(a1), "c"(a2), "d"(a3));
    return ret;
}
static inline int syscall4(int num, int a1, int a2, int a3, int a4) {
    int ret;
    __asm__ volatile("int $0x80" : "=a"(ret) : "a"(num), "b"(a1), "c"(a2), "d"(a3), "S"(a4));
    return ret;
}

static void print(const char *s) {
    int len = 0;
    while (s[len]) len++;
    syscall3(SYS_WRITE, 1, (int)s, len);
}

int _start() {
    char cwd[256];
    char name[256];
    int  isdir = 0;

    syscall2(SYS_GETCWD, (int)cwd, sizeof(cwd));

    int fd = syscall2(SYS_OPEN, (int)cwd, VFS_FLAG_READ | VFS_FLAG_DIR);
    if (fd < 0) {
        print("ls: failed to open directory\n");
        syscall1(SYS_EXIT, 1);
        while(1);
    }

    int index = 0;
    int found = 0;
    while (1) {
        int r = syscall4(SYS_READDIR, fd, index, (int)name, (int)&isdir);
        if (r <= 0) break;
        if (isdir) print("[DIR] ");
        print(name);
        print("\n");
        index++;
        found = 1;
    }

    if (!found) print("(empty directory)\n");
    syscall1(SYS_CLOSE, fd);
    syscall1(SYS_EXIT, 0);
    while(1);
}