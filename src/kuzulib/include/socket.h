/* KuzuOS C Library - socket.h for Tor compatibility */
#ifndef _SOCKET_H
#define _SOCKET_H

#include <stdint.h>
#include <stddef.h>

/* Socket length type */
typedef unsigned int socklen_t;

/* Poll nfds type */
typedef unsigned long nfds_t;

/* File descriptor set type - matches libc definition */
typedef int fd_set;

/* Signed size type */
typedef long ssize_t;

/* Socket address family type */
typedef uint16_t sa_family_t;

/* Socket domain constants */
#define AF_UNSPEC   0
#define AF_UNIX     1
#define AF_LOCAL    1
#define AF_INET     2
#define AF_INET6    10

/* Socket type constants */
#define SOCK_STREAM     1
#define SOCK_DGRAM      2
#define SOCK_RAW        3
#define SOCK_SEQPACKET  5

/* Protocol constants */
#define IPPROTO_IP      0
#define IPPROTO_TCP     6
#define IPPROTO_UDP     17

/* Socket options */
#define SOL_SOCKET      1
#define SO_REUSEADDR    2
#define SO_KEEPALIVE    9
#define SO_LINGER       13
#define SO_RCVBUF       8
#define SO_SNDBUF       7

/* Shutdown constants */
#define SHUT_RD     0
#define SHUT_WR     1
#define SHUT_RDWR   2

/* IPv4 address structure */
struct in_addr {
    uint32_t s_addr;
};

/* INADDR_ANY for binding to all interfaces */
#define INADDR_ANY 0x00000000
#define INADDR_LOOPBACK 0x7f000001
#define INADDR_BROADCAST 0xffffffff

/* Socket address structures */
struct sockaddr {
    uint16_t sa_family;
    char     sa_data[14];
};

/* IPv4 socket address */
struct sockaddr_in {
    uint16_t sin_family;
    uint16_t sin_port;
    uint32_t sin_addr;
    char     sin_zero[8];
};

/* IPv6 socket address - forward declare, Tor defines in inaddr_st.h */
struct sockaddr_in6;

/* Generic socket address storage - forward declare */
struct sockaddr_storage;

/* File descriptor set for select - uses libc definition: typedef int fd_set */
#ifndef FD_SETSIZE
#define FD_SETSIZE 1024
#endif

/* FD_* macros for int-based fd_set */
#define FD_ZERO(fdsetp)     (*(fdsetp) = 0)
#define FD_SET(fd, fdsetp)  (*(fdsetp) |= (1 << (fd)))
#define FD_CLR(fd, fdsetp)  (*(fdsetp) &= ~(1 << (fd)))
#define FD_ISSET(fd, fdsetp) ((*(fdsetp) >> (fd)) & 1)

/* Timeval for select - include from time.h if not defined */
#ifndef _TIME_H
struct timeval {
    long tv_sec;
    long tv_usec;
};
#endif

/* Socket API functions */
int socket(int domain, int type, int protocol);
int bind(int sockfd, const struct sockaddr* addr, socklen_t addrlen);
int listen(int sockfd, int backlog);
int accept(int sockfd, struct sockaddr* addr, socklen_t* addrlen);
int connect(int sockfd, const struct sockaddr* addr, socklen_t addrlen);
ssize_t send(int sockfd, const void* buf, size_t len, int flags);
ssize_t recv(int sockfd, void* buf, size_t len, int flags);
ssize_t sendto(int sockfd, const void* buf, size_t len, int flags,
               const struct sockaddr* dest_addr, socklen_t addrlen);
ssize_t recvfrom(int sockfd, void* buf, size_t len, int flags,
                 struct sockaddr* src_addr, socklen_t* addrlen);
int close(int fd);
int shutdown(int sockfd, int how);

/* Socket options */
int getsockopt(int sockfd, int level, int optname, void* optval, socklen_t* optlen);
int setsockopt(int sockfd, int level, int optname, const void* optval, socklen_t optlen);

/* Select */
int select(int nfds, fd_set* readfds, fd_set* writefds, fd_set* exceptfds, struct timeval* timeout);

/* Non-blocking I/O */
#define O_NONBLOCK 0x800
int fcntl(int fd, int cmd, ...);
int ioctl(int fd, unsigned long request, ...);

/* Poll */
#define POLLIN      0x0001
#define POLLOUT     0x0004
#define POLLERR     0x0008
#define POLLHUP     0x0010
#define POLLNVAL    0x0020

struct pollfd {
    int fd;
    short events;
    short revents;
};

int poll(struct pollfd* fds, nfds_t nfds, int timeout);

/* in6_addr is defined by Tor in inaddr_st.h - do not redefine */

/* Host entry */
struct hostent {
    char*  h_name;
    char** h_aliases;
    int    h_addrtype;
    int    h_length;
    char** h_addr_list;
};

/* Netdb functions */
struct hostent* gethostbyname(const char* name);
struct hostent* gethostbyaddr(const void* addr, size_t len, int type);
int gethostname(char* name, size_t len);

#endif /* _SOCKET_H */
