// mkdir.c - Create directory
#define SYS_WRITE 4
#define SYS_MKDIR 39
#define SYS_EXIT 1

static inline int syscall1(int num, int arg1) {
    int ret;
    __asm__ volatile("int $0x80" : "=a"(ret) : "a"(num), "b"(arg1));
    return ret;
}

static inline int syscall2(int num, int arg1, int arg2) {
    int ret;
    __asm__ volatile("int $0x80" : "=a"(ret) : "a"(num), "b"(arg1), "c"(arg2));
    return ret;
}

static inline int syscall3(int num, int arg1, int arg2, int arg3) {
    int ret;
    __asm__ volatile("int $0x80" : "=a"(ret) : "a"(num), "b"(arg1), "c"(arg2), "d"(arg3));
    return ret;
}

static void print(const char* s) {
    int len = 0;
    while (s[len]) len++;
    syscall3(SYS_WRITE, 1, (int)s, len);
}

static void parse_args(int* argc_out, char*** argv_out) {
    unsigned long* stack_ptr;
    __asm__ volatile("mov %%rsp, %0" : "=r"(stack_ptr));
    
    *argc_out = (int)stack_ptr[0];
    *argv_out = (char**)&stack_ptr[1];
}

int _start() {
    int argc;
    char** argv;
    parse_args(&argc, &argv);
    
    if (argc < 2) {
        print("Usage: mkdir <directory>\n");
        syscall1(SYS_EXIT, 1);
    }
    
    const char* dirname = argv[1];
    
    // Create directory with mode 0755
    int result = syscall2(SYS_MKDIR, (int)dirname, 0755);
    
    if (result < 0) {
        print("mkdir: cannot create directory ");
        print(dirname);
        print("\n");
        syscall1(SYS_EXIT, 1);
    }
    
    syscall1(SYS_EXIT, 0);
    while(1);
}
