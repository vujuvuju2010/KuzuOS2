// ping — ICMP echo tool for KuzuOS

typedef unsigned int uint32_t;

static inline int syscall0(int n) {
    int r; __asm__ volatile("int $0x80":"=a"(r):"a"(n)); return r;
}
static inline int syscall1(int n, int a) {
    int r; __asm__ volatile("int $0x80":"=a"(r):"a"(n),"b"(a)); return r;
}
static inline int syscall3(int n, int a, int b, int c) {
    int r; __asm__ volatile("int $0x80":"=a"(r):"a"(n),"b"(a),"c"(b),"d"(c)); return r;
}

#define SYS_EXIT          1
#define SYS_WRITE         4
#define SYS_GETARGC       260
#define SYS_GETARGV       261
#define SYS_NET_POLL      406
#define SYS_NET_PING      410
#define SYS_NET_PING_WAIT 411

static void print(const char* s) {
    int len = 0;
    while (s[len]) len++;
    syscall3(SYS_WRITE, 1, (int)s, len);
}

static void print_char(char c) {
    syscall3(SYS_WRITE, 1, (int)&c, 1);
}

static void print_uint(unsigned int n) {
    if (n == 0) { print_char('0'); return; }
    char buf[12]; int i = 0;
    while (n) { buf[i++] = '0' + (n % 10); n /= 10; }
    while (i--) print_char(buf[i]);
}

static void print_ip(uint32_t ip) {
    print_uint((ip >> 24) & 0xFF); print_char('.');
    print_uint((ip >> 16) & 0xFF); print_char('.');
    print_uint((ip >>  8) & 0xFF); print_char('.');
    print_uint((ip      ) & 0xFF);
}

static uint32_t parse_ip(const char* s) {
    uint32_t r = 0;
    for (int i = 0; i < 4; i++) {
        unsigned int octet = 0;
        while (*s >= '0' && *s <= '9') { octet = octet * 10 + (*s - '0'); s++; }
        if (*s == '.') s++;
        r = (r << 8) | (octet & 0xFF);
    }
    return r;
}

static unsigned int parse_uint(const char* s) {
    unsigned int n = 0;
    while (*s >= '0' && *s <= '9') { n = n * 10 + (*s - '0'); s++; }
    return n;
}

void _start(void) {
    char arg1[64], arg2[64], arg3[64];
    arg1[0] = arg2[0] = arg3[0] = 0;

    int argc = syscall0(SYS_GETARGC);
    if (argc >= 2) syscall3(SYS_GETARGV, 1, (int)arg1, 64);
    if (argc >= 3) syscall3(SYS_GETARGV, 2, (int)arg2, 64);
    if (argc >= 4) syscall3(SYS_GETARGV, 3, (int)arg3, 64);

    if (argc < 2 || arg1[0] == 0) {
        print("Usage: ping <ip> [port] [count]\n");
        syscall1(SYS_EXIT, 1);
    }

    uint32_t ip    = parse_ip(arg1);
    // arg2 is port — ICMP doesn't use it, just acknowledged
    int count = 4; // default
    if (argc >= 4 && arg3[0] != 0)
        count = (int)parse_uint(arg3);

    print("PING ");
    print_ip(ip);
    print(": ");
    print_uint((unsigned int)count);
    print(" packets\n");

    int sent = 0, received = 0;

    for (int i = 0; i < count; i++) {
        if (syscall1(SYS_NET_PING, (int)ip) < 0) {
            print("ping: send failed (ARP pending?), retrying...\n");
            // poll a bit to let ARP resolve then retry once
            for (int p = 0; p < 50000; p++) syscall1(SYS_NET_POLL, 0);
            if (syscall1(SYS_NET_PING, (int)ip) < 0) {
                print("ping: send failed\n");
                continue;
            }
        }
        sent++;

        int got = syscall1(SYS_NET_PING_WAIT, 1000000);
        print("icmp_seq=");
        print_uint((unsigned int)(i + 1));
        if (got) {
            print(" Reply from ");
            print_ip(ip);
            print("\n");
            received++;
        } else {
            print(" Request timeout\n");
        }
    }

    print("\n--- ");
    print_ip(ip);
    print(" ping statistics ---\n");
    print_uint((unsigned int)sent);
    print(" packets transmitted, ");
    print_uint((unsigned int)received);
    print(" received\n");

    syscall1(SYS_EXIT, 0);
}
