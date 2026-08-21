// ip — network manager tool for KuzuOS
// Usage:
//   ip addr          — show IP, MAC, netmask
//   ip route         — show gateway
//   ip link          — show NIC status
//   ip connect <ip> <port>  — open a TCP connection and send/recv
//   ip arp <ip>      — send ARP request for an IP

// syscall wrappers
static inline int syscall1(int n, int a) {
    int r; __asm__ volatile("int $0x80":"=a"(r):"a"(n),"b"(a)); return r;
}
static inline int syscall3(int n, int a, int b, int c) {
    int r; __asm__ volatile("int $0x80":"=a"(r):"a"(n),"b"(a),"c"(b),"d"(c)); return r;
}
static inline int syscall2(int n, int a, int b) {
    int r; __asm__ volatile("int $0x80":"=a"(r):"a"(n),"b"(a),"c"(b)); return r;
}

#define SYS_WRITE        4
#define SYS_EXIT         1

// must match syscall.h in kernel:
#define SYS_NET_GETINFO    400
#define SYS_NET_CONNECT    401
#define SYS_NET_SEND       402
#define SYS_NET_RECV       403
#define SYS_NET_CLOSE      404
#define SYS_NET_CONNECTED  405
#define SYS_NET_POLL       406
#define SYS_NET_ARP        407

#define SYS_GETARGC      260
#define SYS_GETARGV      261

typedef unsigned char  uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int   uint32_t;

// net_info layout (must match syscall 300 in kernel)
typedef struct {
    uint8_t  mac[6];
    uint8_t  _pad[2];
    uint32_t ip;
    uint32_t gateway;
    uint32_t netmask;
} net_info_t;

// ---- minimal libc ----
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

static void print_hex8(uint8_t v) {
    const char* h = "0123456789abcdef";
    print_char(h[v >> 4]);
    print_char(h[v & 0xF]);
}

static void print_ip(uint32_t ip) {
    // ip is in network byte order
    print_uint((ip >> 24) & 0xFF); print_char('.');
    print_uint((ip >> 16) & 0xFF); print_char('.');
    print_uint((ip >>  8) & 0xFF); print_char('.');
    print_uint((ip      ) & 0xFF);
}

static void print_mac(uint8_t* m) {
    for (int i = 0; i < 6; i++) {
        if (i) print_char(':');
        print_hex8(m[i]);
    }
}

static int str_eq(const char* a, const char* b) {
    while (*a && *b && *a == *b) { a++; b++; }
    return *a == *b;
}

static int str_len(const char* s) {
    int n = 0; while (s[n]) n++; return n;
}

// parse "a.b.c.d" into network-byte-order uint32
static uint32_t parse_ip(const char* s) {
    uint32_t r = 0;
    for (int i = 0; i < 4; i++) {
        unsigned int octet = 0;
        while (*s >= '0' && *s <= '9') { octet = octet * 10 + (*s - '0'); s++; }
        if (*s == '.') s++;
        r = (r << 8) | (octet & 0xFF);
    }
    return r;  // already big-endian after shift
}

static unsigned int parse_uint(const char* s) {
    unsigned int n = 0;
    while (*s >= '0' && *s <= '9') { n = n * 10 + (*s - '0'); s++; }
    return n;
}

// ---- commands ----

static void cmd_addr(void) {
    net_info_t info;
    if (syscall2(SYS_NET_GETINFO, (int)&info, 0) < 0) {
        print("ip: NIC not available\n");
        return;
    }
    print("1: eth0\n");
    print("    link/ether ");
    print_mac(info.mac);
    print("\n");
    print("    inet ");
    print_ip(info.ip);
    print("/");
    // calculate prefix length from netmask
    uint32_t nm = info.netmask;
    int prefix = 0;
    for (int i = 31; i >= 0; i--) { if (nm & (1u << i)) prefix++; else break; }
    print_uint(prefix);
    print(" brd ");
    print_ip(info.ip | ~info.netmask);
    print("\n");
}

static void cmd_route(void) {
    net_info_t info;
    if (syscall2(SYS_NET_GETINFO, (int)&info, 0) < 0) {
        print("ip: NIC not available\n");
        return;
    }
    print("default via ");
    print_ip(info.gateway);
    print(" dev eth0\n");
    // also print the local subnet route
    print_ip(info.ip & info.netmask);
    print("/");
    uint32_t nm = info.netmask;
    int prefix = 0;
    for (int i = 31; i >= 0; i--) { if (nm & (1u << i)) prefix++; else break; }
    print_uint(prefix);
    print(" dev eth0 src ");
    print_ip(info.ip);
    print("\n");
}

static void cmd_link(void) {
    net_info_t info;
    int ok = syscall2(SYS_NET_GETINFO, (int)&info, 0);
    print("1: eth0: <BROADCAST,MULTICAST,UP,LOWER_UP>\n");
    print("    link/ether ");
    if (ok == 0) print_mac(info.mac);
    else         print("00:00:00:00:00:00");
    print(" state ");
    print(ok == 0 ? "UP" : "DOWN");
    print("\n");
}

static void cmd_arp(const char* ip_str) {
    if (!ip_str || str_len(ip_str) == 0) {
        print("usage: ip arp <ip>\n");
        return;
    }
    uint32_t ip = parse_ip(ip_str);
    print("Sending ARP request for ");
    print_ip(ip);
    print("...\n");
    syscall2(SYS_NET_ARP, (int)ip, 0);
    // poll a bit to catch the reply
    for (int i = 0; i < 500; i++) syscall1(SYS_NET_POLL, 0);
    print("Done (check kernel log for ARP reply)\n");
}

static void cmd_connect(const char* ip_str, const char* port_str) {
    if (!ip_str || !port_str) {
        print("usage: ip connect <ip> <port>\n");
        return;
    }
    uint32_t dst_ip   = parse_ip(ip_str);
    uint16_t dst_port = (uint16_t)parse_uint(port_str);
    uint16_t src_port = 49152;  // ephemeral

    print("Connecting to ");
    print_ip(dst_ip);
    print_char(':');
    print_uint(dst_port);
    print("...\n");

    int sock = syscall3(SYS_NET_CONNECT, (int)dst_ip, dst_port, src_port);
    if (sock < 0) {
        print("ip: connect failed (ARP may be pending, try again)\n");
        return;
    }

    // wait for handshake (up to ~10 seconds worth of polls)
    int connected = 0;
    for (int i = 0; i < 500000; i++) {
        syscall1(SYS_NET_POLL, 0);
        if (syscall1(SYS_NET_CONNECTED, sock)) { connected = 1; break; }
    }

    if (!connected) {
        print("ip: connection timed out\n");
        syscall1(SYS_NET_CLOSE, sock);
        return;
    }

    print("Connected! Sending HTTP GET...\n");

    const char* req = "GET / HTTP/1.0\r\nHost: 10.0.2.2\r\nConnection: close\r\n\r\n";
    int req_len = str_len(req);
    syscall3(SYS_NET_SEND, sock, (int)req, req_len);

    // receive response
    char buf[512];
    int total = 0;
    for (int i = 0; i < 100000 && total < 2048; i++) {
        syscall1(SYS_NET_POLL, 0);
        int n = syscall3(SYS_NET_RECV, sock, (int)buf, 511);
        if (n > 0) {
            buf[n] = 0;
            print(buf);
            total += n;
        }
    }

    syscall1(SYS_NET_CLOSE, sock);
    print("\n[connection closed]\n");
}

static void print_usage(void) {
    print("Usage:\n");
    print("  ip addr              show IP address and MAC\n");
    print("  ip route             show routing table\n");
    print("  ip link              show NIC status\n");
    print("  ip arp <ip>          send ARP request\n");
    print("  ip connect <ip> <port>  TCP connect and HTTP GET\n");
}

// ---- entry point ----
void _start(void) {
    char arg1[64], arg2[64], arg3[64];
    arg1[0] = arg2[0] = arg3[0] = 0;

    int argc = syscall1(SYS_GETARGC, 0);
    if (argc >= 2) syscall3(SYS_GETARGV, 1, (int)arg1, 64);
    if (argc >= 3) syscall3(SYS_GETARGV, 2, (int)arg2, 64);
    if (argc >= 4) syscall3(SYS_GETARGV, 3, (int)arg3, 64);

    if (argc < 2 || str_len(arg1) == 0) {
        print_usage();
    } else if (str_eq(arg1, "addr")) {
        cmd_addr();
    } else if (str_eq(arg1, "route")) {
        cmd_route();
    } else if (str_eq(arg1, "link")) {
        cmd_link();
    } else if (str_eq(arg1, "arp")) {
        cmd_arp(arg2);
    } else if (str_eq(arg1, "connect")) {
        cmd_connect(arg2, arg3);
    } else {
        print("ip: unknown command '");
        print(arg1);
        print("'\n");
        print_usage();
    }

    syscall1(SYS_EXIT, 0);
}
