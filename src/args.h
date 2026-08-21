#ifndef ARGS_H
#define ARGS_H

#define SYS_GETARGC 260
#define SYS_GETARGV 261

static inline int _args_sys1(int n, int a) {
    int r; __asm__ volatile("int $0x80":"=a"(r):"a"(n),"b"(a)); return r;
}
static inline int _args_sys3(int n, int a, int b, int c) {
    int r; __asm__ volatile("int $0x80":"=a"(r):"a"(n),"b"(a),"c"(b),"d"(c)); return r;
}

static inline int args_get(char** argv, char argbufs[][256], int max) {
    int argc = _args_sys1(SYS_GETARGC, 0);
    if (argc > max) argc = max;
    for (int i = 0; i < argc; i++) {
        int len = _args_sys3(SYS_GETARGV, i, (int)argbufs[i], 256);
        argbufs[i][len < 256 ? len : 255] = '\0';
        argv[i] = argbufs[i];
    }
    argv[argc] = (char*)0;
    return argc;
}

#endif
