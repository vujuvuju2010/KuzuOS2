#include "z_syscalls.h"

// Syscall numbers
#define SYS_GETPROCESSLIST 1000

// Process states
#define PROCESS_READY 0
#define PROCESS_RUNNING 1
#define PROCESS_BLOCKED 2
#define PROCESS_TERMINATED 3
#define PROCESS_STOPPED 4
#define PROCESS_BACKGROUND 5

// Types
typedef unsigned long long uint64_t;

// Syscall interface
extern int32_t z_syscall(uint64_t syscall_num, uint64_t arg1, uint64_t arg2, 
                         uint64_t arg3, uint64_t arg4, uint64_t arg5, uint64_t arg6);

// Simplified process info for userspace
struct process_info {
    uint32_t pid;
    uint32_t state;
    char name[32];
};

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

static void print_padded(const char* s, int width) {
    int len = strlen(s);
    z_write(1, s, len);
    for (int i = len; i < width; i++) {
        z_write(1, " ", 1);
    }
}

static void print_num_padded(uint32_t n, int width) {
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
    
    // Pad with spaces on the left
    for (int i = pos; i < width; i++) {
        z_write(1, " ", 1);
    }
    z_write(1, buf, pos);
}

static const char* state_to_string(uint32_t state) {
    switch (state) {
        case PROCESS_READY: return "ready";
        case PROCESS_RUNNING: return "running";
        case PROCESS_BLOCKED: return "blocked";
        case PROCESS_TERMINATED: return "terminated";
        case PROCESS_STOPPED: return "stopped";
        case PROCESS_BACKGROUND: return "background";
        default: return "unknown";
    }
}

void _start(void) {
    struct process_info processes[256];
    
    // Get process list via syscall
    int count = z_syscall(SYS_GETPROCESSLIST, (uint64_t)processes, 256, 0, 0, 0, 0);
    
    if (count < 0) {
        print("Error: Failed to get process list\n");
        z_exit(1);
    }
    
    if (count == 0) {
        print("No processes running\n");
        z_exit(0);
    }
    
    // Print header
    print("PID   STATE      NAME\n");
    print("---   -----      ----\n");
    
    // Print each process
    for (int i = 0; i < count; i++) {
        print_num_padded(processes[i].pid, 3);
        print("   ");
        print_padded(state_to_string(processes[i].state), 10);
        print(" ");
        print(processes[i].name);
        print("\n");
    }
    
    z_exit(0);
}
