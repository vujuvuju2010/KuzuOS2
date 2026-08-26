#include "z_syscalls.h"

// Syscall numbers
#define SYS_KILLPROCESS 1001

// Types
typedef unsigned long long uint64_t;

// Syscall interface
extern int32_t z_syscall(uint64_t syscall_num, uint64_t arg1, uint64_t arg2, 
                         uint64_t arg3, uint64_t arg4, uint64_t arg5, uint64_t arg6);

static int strlen(const char* s) {
    int len = 0;
    while (s[len]) len++;
    return len;
}

static void print(const char* s) {
    z_write(1, s, strlen(s));
}

static void print_num(uint32_t n) {
    char buf[16];
    int pos = 0;
    if (n == 0) {
        buf[pos++] = '0';
    } else {
        char tmp[16];
        int tp = 0;
        while (n > 0 && tp < 15) {
            tmp[tp++] = '0' + (n % 10);
            n /= 10;
        }
        while (tp--) buf[pos++] = tmp[tp];
    }
    buf[pos] = 0;
    z_write(1, buf, pos);
}

static int atoi(const char* s) {
    int result = 0;
    int i = 0;
    while (s[i] >= '0' && s[i] <= '9') {
        result = result * 10 + (s[i] - '0');
        i++;
    }
    return result;
}

void _start(int argc, char** argv) {
    if (argc < 2) {
        print("Usage: kill <pid>\n");
        z_exit(1);
    }
    
    uint32_t pid = atoi(argv[1]);
    
    if (pid == 0) {
        print("Error: Invalid PID\n");
        z_exit(1);
    }
    
    // Call kill syscall
    int result = z_syscall(SYS_KILLPROCESS, pid, 0, 0, 0, 0, 0);
    
    if (result == 0) {
        print("Process ");
        print_num(pid);
        print(" killed\n");
        z_exit(0);
    } else if (result == -1) {
        print("Error: Process ");
        print_num(pid);
        print(" not found\n");
        z_exit(1);
    } else if (result == -3) {
        print("Error: Cannot kill init process\n");
        z_exit(1);
    } else {
        print("Error: Failed to kill process ");
        print_num(pid);
        print("\n");
        z_exit(1);
    }
}
