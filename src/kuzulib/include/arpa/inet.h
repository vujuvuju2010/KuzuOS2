/* KuzuOS C Library - arpa/inet.h for Tor compatibility */
#ifndef _ARPA_INET_H
#define _ARPA_INET_H

#include <stdint.h>
#include <stddef.h>

/* Forward declarations */
struct in_addr {
    uint32_t s_addr;
};

/* Byte order conversion - x86_64 is little endian */
static inline uint32_t htonl(uint32_t hostlong) {
    return ((hostlong & 0x000000FF) << 24) |
           ((hostlong & 0x0000FF00) << 8) |
           ((hostlong & 0x00FF0000) >> 8) |
           ((hostlong & 0xFF000000) >> 24);
}

static inline uint16_t htons(uint16_t hostshort) {
    return ((hostshort & 0x00FF) << 8) |
           ((hostshort & 0xFF00) >> 8);
}

static inline uint32_t ntohl(uint32_t netlong) {
    return ((netlong & 0x000000FF) << 24) |
           ((netlong & 0x0000FF00) << 8) |
           ((netlong & 0x00FF0000) >> 8) |
           ((netlong & 0xFF000000) >> 24);
}

static inline uint16_t ntohs(uint16_t netshort) {
    return ((netshort & 0x00FF) << 8) |
           ((netshort & 0xFF00) >> 8);
}

/* Address conversion */
int inet_pton(int af, const char *src, void *dst);
const char *inet_ntop(int af, const void *src, char *dst, size_t size);

/* Legacy functions */
uint32_t inet_addr(const char *cp);
char *inet_ntoa(struct in_addr in);

#endif /* _ARPA_INET_H */
