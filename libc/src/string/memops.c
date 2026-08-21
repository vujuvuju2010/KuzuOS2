/* libc/src/string/memops.c */
#include <string.h>

void* memcpy(void* dest, const void* src, size_t n) {
    unsigned char* d = (unsigned char*)dest;
    const unsigned char* s = (const unsigned char*)src;
    for (size_t i = 0; i < n; i++)
        d[i] = s[i];
    return dest;
}

void* memmove(void* dest, const void* src, size_t n) {
    unsigned char* d = (unsigned char*)dest;
    const unsigned char* s = (const unsigned char*)src;
    if (dest < src) {
        for (size_t i = 0; i < n; i++)
            d[i] = s[i];
    } else {
        for (int i = (int)n - 1; i >= 0; i--)
            d[i] = s[i];
    }
    return dest;
}

void* memset(void* s, int c, size_t n) {
    unsigned char* p = (unsigned char*)s;
    unsigned char val = (unsigned char)c;
    for (size_t i = 0; i < n; i++)
        p[i] = val;
    return s;
}

int memcmp(const void* s1, const void* s2, size_t n) {
    const unsigned char* p1 = (const unsigned char*)s1;
    const unsigned char* p2 = (const unsigned char*)s2;
    for (size_t i = 0; i < n; i++) {
        if (p1[i] != p2[i])
            return p1[i] - p2[i];
    }
    return 0;
}

void* memchr(const void* s, int c, size_t n) {
    const unsigned char* p = (const unsigned char*)s;
    unsigned char val = (unsigned char)c;
    for (size_t i = 0; i < n; i++) {
        if (p[i] == val)
            return (void*)(p + i);
    }
    return NULL;
}

char* strchr(const char* s, int c) {
    unsigned char ch = (unsigned char)c;
    while (*s && (unsigned char)*s != ch)
        s++;
    return ((unsigned char)*s == ch) ? (char*)s : NULL;
}

char* strrchr(const char* s, int c) {
    unsigned char ch = (unsigned char)c;
    const char* result = NULL;
    while (*s) {
        if ((unsigned char)*s == ch)
            result = s;
        s++;
    }
    return (char*)result;
}

char* strstr(const char* haystack, const char* needle) {
    size_t nlen = strlen(needle);
    if (nlen == 0) return (char*)haystack;
    
    while (*haystack) {
        if (strncmp(haystack, needle, nlen) == 0)
            return (char*)haystack;
        haystack++;
    }
    return NULL;
}
