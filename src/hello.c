void _start() {
    // syscall to write "hello\n" to stdout
    // write(1, "hello\n", 6)
    __asm__ volatile(
        "mov $4, %%eax\n"      // SYS_WRITE = 4
        "mov $1, %%ebx\n"      // fd = 1 (stdout)
        "mov $msg, %%ecx\n"    // buf = msg
        "mov $6, %%edx\n"      // count = 6
        "int $0x80\n"
        ::: "eax", "ebx", "ecx", "edx"
    );
    
    // syscall to exit(0)
    __asm__ volatile(
        "mov $1, %%eax\n"      // SYS_EXIT = 1
        "mov $0, %%ebx\n"      // status = 0
        "int $0x80\n"
        ::: "eax", "ebx"
    );
}

static const char msg[] = "hello\n";
