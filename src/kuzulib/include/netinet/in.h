/* KuzuOS C Library - netinet/in.h for Tor compatibility */
#ifndef _NETINET_IN_H
#define _NETINET_IN_H

#include <stdint.h>
#include <stddef.h>

/* Internet address family */
#define AF_INET     2
#define AF_INET6    10
#define AF_UNSPEC   0

/* Protocol numbers */
#define IPPROTO_IP      0
#define IPPROTO_ICMP    1
#define IPPROTO_TCP     6
#define IPPROTO_UDP     17
#define IPPROTO_IPV6    41

/* Socket address structures */
struct in_addr {
    uint32_t s_addr;
};

struct sockaddr_in {
    uint16_t sin_family;
    uint16_t sin_port;
    struct in_addr sin_addr;
    char        sin_zero[8];
};

struct in6_addr {
    union {
        uint8_t  s6_addr8[16];
        uint16_t s6_addr16[8];
        uint32_t s6_addr32[4];
    } __in6_union;
};
#define s6_addr   __in6_union.s6_addr8
#define s6_addr16 __in6_union.s6_addr16
#define s6_addr32 __in6_union.s6_addr32

struct sockaddr_in6 {
    uint16_t     sin6_family;
    uint16_t     sin6_port;
    uint32_t     sin6_flowinfo;
    struct in6_addr sin6_addr;
    uint32_t     sin6_scope_id;
};

/* IPv6 address macros */
#define IN6ADDR_ANY_INIT        { { { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 } } }
#define IN6ADDR_LOOPBACK_INIT   { { { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1 } } }

/* Multicast macros */
#define IN_MULTICAST(a)         (((uint32_t)(a) & 0xf0000000) == 0xe0000000)
#define IN_EXPERIMENTAL(a)      (((uint32_t)(a) & 0xe0000000) == 0xe0000000)
#define IN_LOOPBACK(a)          (((uint32_t)(a) & 0xff000000) == 0x7f000000)

/* Special addresses */
#define INADDR_ANY      ((uint32_t)0x00000000)
#define INADDR_BROADCAST ((uint32_t)0xffffffff)
#define INADDR_NONE     ((uint32_t)0xffffffff)
#define INADDR_LOOPBACK ((uint32_t)0x7f000000)

/* IPv6 special addresses */
#define IN6_IS_ADDR_UNSPECIFIED(a)  \
    (((const uint32_t *)(a))[0] == 0 && ((const uint32_t *)(a))[1] == 0 && \
     ((const uint32_t *)(a))[2] == 0 && ((const uint32_t *)(a))[3] == 0)

#define IN6_IS_ADDR_LOOPBACK(a)  \
    (((const uint32_t *)(a))[0] == 0 && ((const uint32_t *)(a))[1] == 0 && \
     ((const uint32_t *)(a))[2] == 0 && \
     ((const uint8_t *)(a))[12] == 0 && ((const uint8_t *)(a))[13] == 0 && \
     ((const uint8_t *)(a))[14] == 0 && ((const uint8_t *)(a))[15] == 1)

#define IN6_IS_ADDR_MULTICAST(a)    (((const uint8_t *)(a))[0] == 0xff)

/* Socket options */
#define IPV6_V6ONLY     26

#endif /* _NETINET_IN_H */
