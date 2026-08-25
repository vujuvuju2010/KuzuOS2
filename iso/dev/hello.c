void _start() {
    __asm__ volatile(
        "movl $4, %%eax\n"
        "movl $1, %%ebx\n"
        "movl $msg, %%ecx\n"
        "movl $6, %%edx\n"
        "int $0x80\n"
        "movl $1, %%eax\n"
        "movl $0, %%ebx\n"
        "int $0x80\n"
        ::: "eax", "ebx", "ecx", "edx"
    );
}

__asm__(
    "msg: .ascii \"hello\\n\""
);
