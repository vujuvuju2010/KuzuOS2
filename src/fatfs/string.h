#ifndef KUZUOS_STRING_H
#define KUZUOS_STRING_H

static inline int strlen(const char* str) {
    int len = 0;
    while (str[len] != '\0') len++;
    return len;
}

static inline void* memset(void* s, int c, unsigned n) {
    unsigned char* p = s;
    while (n--) *p++ = (unsigned char)c;
    return s;
}
static inline void* memcpy(void* d, const void* s, unsigned n) {
    unsigned char* dp = d;
    const unsigned char* sp = s;
    while (n--) *dp++ = *sp++;
    return d;
}
static inline int memcmp(const void* s1, const void* s2, unsigned n) {
    const unsigned char* p1 = s1;
    const unsigned char* p2 = s2;
    while (n--) if (*p1++ != *p2++) return p1[-1] - p2[-1];
    return 0;
}
static inline char* strchr(const char* s, int c) {
    while (*s) { if (*s == (char)c) return (char*)s; s++; } return 0;
}
static inline char* strncat(char* dest, const char* src, unsigned n) {
    char* d = dest + strlen(dest);
    while (n-- && *src) *d++ = *src++;
    *d = 0;
    return dest;
}

#endif // KUZUOS_STRING_H 