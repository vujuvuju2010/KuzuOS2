// servicectl.c - Service management command for KuzuOS2
// Manages services defined in /etc/services/*.conf

#include "z_syscalls.h"

// Type definitions
typedef unsigned long long uint64_t;
typedef long long int64_t;

// Syscall numbers for service management
#define SYS_SERVICE_START    1010
#define SYS_SERVICE_STOP     1011
#define SYS_SERVICE_RESTART  1012
#define SYS_SERVICE_STATUS   1013
#define SYS_SERVICE_LIST     1014

// Syscall interface
extern int64_t z_syscall(uint64_t syscall_num, uint64_t arg1, uint64_t arg2,
                         uint64_t arg3, uint64_t arg4, uint64_t arg5, uint64_t arg6);

// Simple string length
static int str_len(const char* s) {
    int len = 0;
    while (s[len]) len++;
    return len;
}

// Simple string compare
static int str_cmp(const char* s1, const char* s2) {
    while (*s1 && *s2) {
        if ((unsigned char)*s1 != (unsigned char)*s2)
            return (unsigned char)*s1 - (unsigned char)*s2;
        s1++;
        s2++;
    }
    return (unsigned char)*s1 - (unsigned char)*s2;
}

// Simple string copy
static char* str_cpy(char* dest, const char* src) {
    char* orig = dest;
    while (*src) {
        *dest++ = *src++;
    }
    *dest = '\0';
    return orig;
}

// Simple print
static void print(const char* s) {
    z_write(1, s, str_len(s));
}

// Print integer
static void print_int(int n) {
    char buf[16];
    int i = 0;
    int neg = 0;
    
    if (n < 0) {
        neg = 1;
        n = -n;
    }
    
    do {
        buf[i++] = (n % 10) + '0';
        n /= 10;
    } while (n > 0);
    
    if (neg) {
        buf[i++] = '-';
    }
    
    // Reverse and print
    while (i > 0) {
        char c = buf[--i];
        z_write(1, &c, 1);
    }
}

static void print_usage(void) {
    print("Usage: servicectl <command> [service_name]\n");
    print("\n");
    print("Commands:\n");
    print("  list              List all services\n");
    print("  status <name>     Show status of a service\n");
    print("  start <name>      Start a service\n");
    print("  stop <name>       Stop a service\n");
    print("  restart <name>    Restart a service\n");
    print("\n");
    print("Examples:\n");
    print("  servicectl list\n");
    print("  servicectl status httpd\n");
    print("  servicectl start httpd\n");
    print("  servicectl stop httpd\n");
    print("  servicectl restart httpd\n");
}

static int do_list(void) {
    char buffer[4096];
    int64_t result = z_syscall(SYS_SERVICE_LIST, (uint64_t)buffer, (uint64_t)4096, 0, 0, 0, 0);
    if (result < 0) {
        print("servicectl: failed to list services (error ");
        print_int((int)result);
        print(")\n");
        return 1;
    }
    print(buffer);
    return 0;
}

static int do_status(const char* name) {
    if (!name) {
        print("servicectl: service name required for status\n");
        return 1;
    }
    
    char buffer[1024];
    int64_t result = z_syscall(SYS_SERVICE_STATUS, (uint64_t)name, (uint64_t)buffer, (uint64_t)1024, 0, 0, 0);
    if (result < 0) {
        print("servicectl: failed to get status for '");
        print(name);
        print("' (error ");
        print_int((int)result);
        print(")\n");
        return 1;
    }
    print(buffer);
    return 0;
}

static int do_start(const char* name) {
    if (!name) {
        print("servicectl: service name required for start\n");
        return 1;
    }
    
    int64_t result = z_syscall(SYS_SERVICE_START, (uint64_t)name, 0, 0, 0, 0, 0);
    if (result < 0) {
        print("servicectl: failed to start '");
        print(name);
        print("' (error ");
        print_int((int)result);
        print(")\n");
        return 1;
    }
    print("servicectl: service '");
    print(name);
    print("' started\n");
    return 0;
}

static int do_stop(const char* name) {
    if (!name) {
        print("servicectl: service name required for stop\n");
        return 1;
    }
    
    int64_t result = z_syscall(SYS_SERVICE_STOP, (uint64_t)name, 0, 0, 0, 0, 0);
    if (result < 0) {
        print("servicectl: failed to stop '");
        print(name);
        print("' (error ");
        print_int((int)result);
        print(")\n");
        return 1;
    }
    print("servicectl: service '");
    print(name);
    print("' stopped\n");
    return 0;
}

static int do_restart(const char* name) {
    if (!name) {
        print("servicectl: service name required for restart\n");
        return 1;
    }
    
    int64_t result = z_syscall(SYS_SERVICE_RESTART, (uint64_t)name, 0, 0, 0, 0, 0);
    if (result < 0) {
        print("servicectl: failed to restart '");
        print(name);
        print("' (error ");
        print_int((int)result);
        print(")\n");
        return 1;
    }
    print("servicectl: service '");
    print(name);
    print("' restarted\n");
    return 0;
}

void _start(unsigned long long argc_val, unsigned long long argv_array_addr) {
    // In the System V AMD64 ABI for _start:
    // - argc_val contains the actual argc value
    // - argv_array_addr contains the address where argv pointers are stored on stack
    //   argv_array_addr points to argv[0], argv_array_addr+8 points to argv[1], etc.
    
    int argc = (int)argc_val;
    unsigned long long* argv_arr = (unsigned long long*)argv_array_addr;
    
    // Validate argc first - must be at least 1 (program name)
    if (argc < 1 || argc > 100) {
        print("servicectl: invalid argc=");
        print_int(argc);
        print("\n");
        z_exit(1);
    }
    
    // Check for minimum args for commands
    if (argc < 2) {
        print_usage();
        z_exit(1);
    }
    
    // Get argv[0] which is the program name (e.g., "servicectl")
    // argv_arr[0] gives us the pointer stored at argv_array_addr
    const char* prog = (const char*)argv_arr[0];
    
    // Get argv[1] which is the command (e.g., "list", "start")
    // argv_arr[1] gives us the pointer stored at argv_array_addr + 8
    const char* cmd = (const char*)argv_arr[1];
    
    // Validate cmd pointer
    if ((unsigned long long)cmd < 0x400000 || (unsigned long long)cmd > 0xFFFFFFFF) {
        print("servicectl: invalid cmd pointer=");
        print_int((int)(unsigned long long)cmd);
        print("\n");
        z_exit(1);
    }
    
    const char* service_name = (argc > 2) ? (const char*)argv_arr[2] : (const char*)0;
    
    if (str_cmp(cmd, "list") == 0) {
        z_exit(do_list());
    } else if (str_cmp(cmd, "status") == 0) {
        z_exit(do_status(service_name));
    } else if (str_cmp(cmd, "start") == 0) {
        z_exit(do_start(service_name));
    } else if (str_cmp(cmd, "stop") == 0) {
        z_exit(do_stop(service_name));
    } else if (str_cmp(cmd, "restart") == 0) {
        z_exit(do_restart(service_name));
    } else if (str_cmp(cmd, "help") == 0 || str_cmp(cmd, "--help") == 0) {
        print_usage();
        z_exit(0);
    } else {
        print("servicectl: unknown command '");
        print(cmd);
        print("'\n");
        print_usage();
        z_exit(1);
    }
    
    z_exit(0);
}
