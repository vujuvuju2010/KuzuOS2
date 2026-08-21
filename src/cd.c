// cd.c - Change directory
#define SYS_CHDIR  12
#define SYS_WRITE   4
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

void _start(void) {
    char path[256];
    int argc = _sys1(SYS_GETARGC, 0);

    if (argc < 2) {
        print("cd: missing argument\n");
        _sys1(SYS_EXIT, 1);
    }

    _sys3(SYS_GETARGV, 1, (int)path, 256);

    if (_sys1(SYS_CHDIR, (int)path) < 0) {
        print("cd: no such directory: ");
        print(path);
        print("\n");
        _sys1(SYS_EXIT, 1);
    }

    _sys1(SYS_EXIT, 0);
}
