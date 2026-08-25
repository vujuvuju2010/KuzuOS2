// mkdir.c - Create directory
#define SYS_WRITE   4
#define SYS_MKDIR   39
#define SYS_EXIT    1
#define SYS_GETARGC 260
#define SYS_GETARGV 261

static inline int _sys1(int n, int a) {
    int r; __asm__ volatile("int $0x80":"=a"(r):"a"(n),"b"(a)); return r;
}
static inline int _sys3(int n, int a, int b, int c) {
    int r; __asm__ volatile("int $0x80":"=a"(r):"a"(n),"b"(a),"c"(b),"d"(c)); return r;
}

static void print(const char* s) {
    int len = 0; while (s[len]) len++;
    _sys3(SYS_WRITE, 1, (int)s, len);
}

int _start(void) {
    char path[256];

    print("[mkdir] starting\n");
    int argc = _sys1(SYS_GETARGC, 0);
    
    if (argc < 2) {
        print("Usage: mkdir <directory>\n");
        _sys1(SYS_EXIT, 1);
        while(1);
    }

    _sys3(SYS_GETARGV, 1, (int)path, 256);

    print("[mkdir] about to call SYS_MKDIR syscall\n");
    
    int result = _sys1(SYS_MKDIR, (int)path);
    
    print("[mkdir] syscall returned result=");
    if (result == 0) print("OK");
    else if (result == -1) print("-1");
    else print("ERR");
    print("\n");
    
    if (result < 0) {
        print("mkdir: cannot create directory ");
        print(path);
        print("\n");
        _sys1(SYS_EXIT, 1);
        while(1);
    }

    print("[mkdir] exiting normally\n");
    _sys1(SYS_EXIT, 0);
    while(1);
}
