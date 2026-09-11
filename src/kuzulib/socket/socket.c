/* KuzuOS C Library - socket.c implementation for Tor compatibility */
#include <socket.h>
#include <stdint.h>
#include <string.h>

/* Syscall numbers for network operations */
#define SYS_NET_SOCKET      401
#define SYS_NET_SEND        402
#define SYS_NET_RECV        403
#define SYS_NET_CLOSE       404
#define SYS_NET_BIND        405
#define SYS_NET_POLL        406
#define SYS_NET_CONNECT     415
#define SYS_NET_LISTEN      413
#define SYS_NET_ACCEPT      414

/* Simple syscall wrappers */
static inline int syscall1(int n, int a) {
    int r;
    __asm__ volatile("int $0x80" : "=a"(r) : "a"(n), "b"(a));
    return r;
}

static inline int syscall2(int n, int a, int b) {
    int r;
    __asm__ volatile("int $0x80" : "=a"(r) : "a"(n), "b"(a), "c"(b));
    return r;
}

static inline int syscall3(int n, int a, int b, int c) {
    int r;
    __asm__ volatile("int $0x80" : "=a"(r) : "a"(n), "b"(a), "c"(b), "d"(c));
    return r;
}

static inline int syscall4(int n, int a, int b, int c, int d) {
    int r;
    __asm__ volatile("int $0x80" : "=a"(r) : "a"(n), "b"(a), "c"(b), "d"(c), "S"(d));
    return r;
}

/* Create a socket */
int socket(int domain, int type, int protocol) {
    (void)domain;
    (void)protocol;
    /* For now, just return a new socket based on type */
    if (type == SOCK_STREAM) {
        /* TCP socket */
        return syscall1(SYS_NET_SOCKET, 6);  /* IPPROTO_TCP */
    } else if (type == SOCK_DGRAM) {
        /* UDP socket */
        return syscall1(SYS_NET_SOCKET, 17); /* IPPROTO_UDP */
    }
    return -1;
}

/* Bind socket to address */
int bind(int sockfd, const struct sockaddr* addr, socklen_t addrlen) {
    (void)addrlen;
    if (!addr) return -1;
    
    /* Extract port from sockaddr_in */
    const struct sockaddr_in* sin = (const struct sockaddr_in*)addr;
    uint16_t port = sin->sin_port;  /* Already in network byte order */
    
    return syscall2(SYS_NET_BIND, sockfd, (int)port);
}

/* Listen for connections */
int listen(int sockfd, int backlog) {
    (void)backlog;
    /* Use our NET_LISTEN syscall */
    /* Note: our syscall takes port, but for bound socket we need different approach */
    return syscall1(SYS_NET_LISTEN, sockfd);
}

/* Accept a connection */
int accept(int sockfd, struct sockaddr* addr, socklen_t* addrlen) {
    (void)addr;
    (void)addrlen;
    return syscall1(SYS_NET_ACCEPT, sockfd);
}

/* Connect to remote address */
int connect(int sockfd, const struct sockaddr* addr, socklen_t addrlen) {
    (void)addrlen;
    if (!addr) return -1;
    
    const struct sockaddr_in* sin = (const struct sockaddr_in*)addr;
    uint32_t ip = sin->sin_addr;    /* Network byte order */
    uint16_t port = sin->sin_port;  /* Network byte order */
    
    return syscall3(SYS_NET_CONNECT, (int)ip, (int)port, sockfd);
}

/* Send data on socket */
ssize_t send(int sockfd, const void* buf, size_t len, int flags) {
    (void)flags;
    return syscall3(SYS_NET_SEND, sockfd, (int)buf, (int)len);
}

/* Receive data from socket */
ssize_t recv(int sockfd, void* buf, size_t len, int flags) {
    (void)flags;
    return syscall3(SYS_NET_RECV, sockfd, (int)buf, (int)len);
}

/* Send data to specific address */
ssize_t sendto(int sockfd, const void* buf, size_t len, int flags,
               const struct sockaddr* dest_addr, socklen_t addrlen) {
    (void)flags;
    (void)dest_addr;
    (void)addrlen;
    /* For UDP - would need to handle destination address */
    return syscall3(SYS_NET_SEND, sockfd, (int)buf, (int)len);
}

/* Receive data from specific address */
ssize_t recvfrom(int sockfd, void* buf, size_t len, int flags,
                 struct sockaddr* src_addr, socklen_t* addrlen) {
    (void)flags;
    (void)src_addr;
    (void)addrlen;
    return syscall3(SYS_NET_RECV, sockfd, (int)buf, (int)len);
}

/* Close socket */
int close(int fd) {
    return syscall1(SYS_NET_CLOSE, fd);
}

/* Shutdown socket */
int shutdown(int sockfd, int how) {
    (void)sockfd;
    (void)how;
    /* For now, just close */
    return 0;
}

/* Get socket option */
int getsockopt(int sockfd, int level, int optname, void* optval, socklen_t* optlen) {
    (void)sockfd;
    (void)level;
    (void)optname;
    (void)optval;
    (void)optlen;
    /* Stub - return success */
    return 0;
}

/* Set socket option */
int setsockopt(int sockfd, int level, int optname, const void* optval, socklen_t optlen) {
    (void)sockfd;
    (void)level;
    (void)optname;
    (void)optval;
    (void)optlen;
    /* Stub - return success */
    return 0;
}

/* Select - wait for I/O readiness */
int select(int nfds, fd_set* readfds, fd_set* writefds, fd_set* exceptfds, struct timeval* timeout) {
    (void)nfds;
    (void)readfds;
    (void)writefds;
    (void)exceptfds;
    (void)timeout;
    /* Stub - use poll instead */
    return 0;
}

/* Poll - wait for I/O events */
int poll(struct pollfd* fds, nfds_t nfds, int timeout) {
    (void)fds;
    (void)nfds;
    (void)timeout;
    /* Use our NET_POLL syscall */
    return syscall1(SYS_NET_POLL, 0);
}

/* Fcntl - file control */
int fcntl(int fd, int cmd, ...) {
    (void)fd;
    (void)cmd;
    /* Stub for non-blocking mode */
    return 0;
}

/* Ioctl - I/O control */
int ioctl(int fd, unsigned long request, ...) {
    (void)fd;
    (void)request;
    return 0;
}

/* Get host by name - stub */
struct hostent* gethostbyname(const char* name) {
    static struct hostent ent;
    static char* aliases[1] = {NULL};
    static char* addrs[2] = {NULL, NULL};
    static struct in_addr addr;
    
    if (!name) return NULL;
    
    /* Simple stub - would need DNS resolver */
    ent.h_name = (char*)name;
    ent.h_aliases = aliases;
    ent.h_addrtype = AF_INET;
    ent.h_length = sizeof(struct in_addr);
    ent.h_addr_list = addrs;
    
    /* Try to parse as IP */
    addr.s_addr = inet_addr(name);
    addrs[0] = (char*)&addr;
    
    return &ent;
}

/* Get host by address - stub */
struct hostent* gethostbyaddr(const void* addr, size_t len, int type) {
    (void)addr;
    (void)len;
    (void)type;
    return NULL;
}

/* Get hostname */
int gethostname(char* name, size_t len) {
    if (len < 6) return -1;
    name[0] = 'k'; name[1] = 'u'; name[2] = 'z'; name[3] = 'u'; name[4] = 's'; name[5] = '\0';
    return 0;
}

/* inet_ntoa - convert network address to string */
const char* inet_ntoa(struct in_addr in) {
    static char buf[16];
    uint32_t addr = in.s_addr;
    uint8_t* b = (uint8_t*)&addr;
    
    /* Note: addr is in network byte order, so b[0] is first octet */
    int p = 0;
    uint32_t n;
    
    /* Octet 1 */
    n = b[0];
    if (n >= 100) { buf[p++] = '0' + (n / 100); n %= 100; }
    if (n >= 10) { buf[p++] = '0' + (n / 10); n %= 10; }
    buf[p++] = '0' + n;
    buf[p++] = '.';
    
    /* Octet 2 */
    n = b[1];
    if (n >= 100) { buf[p++] = '0' + (n / 100); n %= 100; }
    if (n >= 10) { buf[p++] = '0' + (n / 10); n %= 10; }
    buf[p++] = '0' + n;
    buf[p++] = '.';
    
    /* Octet 3 */
    n = b[2];
    if (n >= 100) { buf[p++] = '0' + (n / 100); n %= 100; }
    if (n >= 10) { buf[p++] = '0' + (n / 10); n %= 10; }
    buf[p++] = '0' + n;
    buf[p++] = '.';
    
    /* Octet 4 */
    n = b[3];
    if (n >= 100) { buf[p++] = '0' + (n / 100); n %= 100; }
    if (n >= 10) { buf[p++] = '0' + (n / 10); n %= 10; }
    buf[p++] = '0' + n;
    
    buf[p] = '\0';
    return buf;
}

/* inet_aton - convert string to network address */
int inet_aton(const char* cp, struct in_addr* inp) {
    if (!cp || !inp) return 0;
    
    uint32_t addr = 0;
    uint8_t* b = (uint8_t*)&addr;
    int octet = 0;
    uint32_t val = 0;
    
    while (*cp && octet < 4) {
        if (*cp == '.') {
            b[octet++] = (uint8_t)val;
            val = 0;
        } else if (*cp >= '0' && *cp <= '9') {
            val = val * 10 + (*cp - '0');
            if (val > 255) return 0;
        } else {
            return 0;
        }
        cp++;
    }
    
    if (octet != 3) return 0;
    b[3] = (uint8_t)val;
    
    inp->s_addr = addr;  /* Already in network byte order */
    return 1;
}

/* inet_addr - convert string to network address (legacy) */
uint32_t inet_addr(const char* cp) {
    struct in_addr addr;
    if (inet_aton(cp, &addr)) {
        return addr.s_addr;
    }
    return 0xFFFFFFFF;  /* INADDR_NONE */
}

/* errno - thread-local in real systems, global here */
int errno = 0;
