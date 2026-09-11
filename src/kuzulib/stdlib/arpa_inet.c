#include <arpa/inet.h>
#include <socket.h>
#include <string.h>

uint32_t inet_addr(const char *cp) {
    uint32_t addr = 0;
    uint32_t byte;
    int i;
    
    for (i = 0; i < 4; i++) {
        byte = 0;
        while (*cp >= '0' && *cp <= '9') {
            byte = byte * 10 + (*cp - '0');
            cp++;
        }
        if (*cp == '.') cp++;
        addr |= (byte << (24 - i * 8));
    }
    return addr;
}

char *inet_ntoa(struct in_addr in) {
    static char buf[16];
    unsigned char *bytes = (unsigned char *)&in.s_addr;
    __builtin_sprintf(buf, "%u.%u.%u.%u", bytes[0], bytes[1], bytes[2], bytes[3]);
    return buf;
}

int inet_pton(int af, const char *src, void *dst) {
    if (af != AF_INET) return -1;
    
    uint32_t addr = inet_addr(src);
    if (addr == 0 && strcmp(src, "0.0.0.0") != 0) {
        return 0;
    }
    *(uint32_t *)dst = addr;
    return 1;
}

const char *inet_ntop(int af, const void *src, char *dst, size_t size) {
    if (af != AF_INET) return NULL;
    if (size < 16) return NULL;
    
    const unsigned char *bytes = (const unsigned char *)src;
    __builtin_sprintf(dst, "%u.%u.%u.%u", bytes[0], bytes[1], bytes[2], bytes[3]);
    return dst;
}
