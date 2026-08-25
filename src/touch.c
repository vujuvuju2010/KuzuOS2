// touch.c - Create empty file or update timestamp
#define SYS_WRITE 4
#define SYS_OPEN 5
#define SYS_CLOSE 6
#define SYS_EXIT 1
#define SYS_GETARGC 260
#define SYS_GETARGV 261

#define O_WRONLY 0x01
#define O_CREAT  0x40
#define O_TRUNC  0x200

static inline int syscall1(int num, int arg1) {
    int ret;
    __asm__ volatile("int $0x80" : "=a"(ret) : "a"(num), "b"(arg1) : "memory");
    return ret;
}

static inline int syscall2(int num, int arg1, int arg2) {
    int ret;
    __asm__ volatile("int $0x80" : "=a"(ret) : "a"(num), "b"(arg1), "c"(arg2) : "memory");
    return ret;
}

static inline int syscall3(int num, int arg1, int arg2, int arg3) {
    int ret;
    __asm__ volatile("int $0x80" : "=a"(ret) : "a"(num), "b"(arg1), "c"(arg2), "d"(arg3) : "memory");
    return ret;
}

static void print(const char* s) {
    int len = 0;
    while (s[len]) len++;
    syscall3(SYS_WRITE, 1, (int)s, len);
}

int _start(void) {
    char filename[256];
    
    int argc = syscall1(SYS_GETARGC, 0);
    
    if (argc < 2) {
        print("Usage: touch <file>\n");
        syscall1(SYS_EXIT, 1);
    }
    
    syscall3(SYS_GETARGV, 1, (int)filename, 256);
    
    // Try to create/truncate file with O_CREAT flag
    int fd = syscall2(SYS_OPEN, (int)filename, O_WRONLY | O_CREAT);
    
    if (fd < 0) {
        print("touch: cannot touch '");
        print(filename);
        print("'\n");
        syscall1(SYS_EXIT, 1);
    }
    
    // Close the file
    syscall1(SYS_CLOSE, fd);
    
    syscall1(SYS_EXIT, 0);
}
