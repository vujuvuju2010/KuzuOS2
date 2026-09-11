/* KuzuOS C Library - string.c implementation */
#include <string.h>
#include <stdint.h>

void* memcpy(void* dest, const void* src, size_t n) {
    unsigned char* d = (unsigned char*)dest;
    const unsigned char* s = (const unsigned char*)src;
    while (n--) *d++ = *s++;
    return dest;
}

void* memmove(void* dest, const void* src, size_t n) {
    unsigned char* d = (unsigned char*)dest;
    const unsigned char* s = (const unsigned char*)src;
    
    if (d < s) {
        while (n--) *d++ = *s++;
    } else {
        d += n;
        s += n;
        while (n--) *--d = *--s;
    }
    return dest;
}

void* memset(void* s, int c, size_t n) {
    unsigned char* p = (unsigned char*)s;
    while (n--) *p++ = (unsigned char)c;
    return s;
}

int memcmp(const void* s1, const void* s2, size_t n) {
    const unsigned char* p1 = (const unsigned char*)s1;
    const unsigned char* p2 = (const unsigned char*)s2;
    while (n--) {
        if (*p1 != *p2) return *p1 - *p2;
        p1++; p2++;
    }
    return 0;
}

void* memchr(const void* s, int c, size_t n) {
    const unsigned char* p = (const unsigned char*)s;
    while (n--) {
        if (*p == (unsigned char)c) return (void*)p;
        p++;
    }
    return NULL;
}

size_t strlen(const char* s) {
    size_t len = 0;
    while (s[len]) len++;
    return len;
}

size_t strnlen(const char* s, size_t maxlen) {
    size_t len = 0;
    while (len < maxlen && s[len]) len++;
    return len;
}

char* strcpy(char* dest, const char* src) {
    char* d = dest;
    while ((*d++ = *src++));
    return dest;
}

char* strncpy(char* dest, const char* src, size_t n) {
    char* d = dest;
    while (n && (*d++ = *src++)) n--;
    while (n--) *d++ = '\0';
    return dest;
}

char* strcat(char* dest, const char* src) {
    char* d = dest;
    while (*d) d++;
    while ((*d++ = *src++));
    return dest;
}

char* strncat(char* dest, const char* src, size_t n) {
    char* d = dest;
    while (*d) d++;
    while (n-- && (*d++ = *src++));
    if (!*src) d[-1] = '\0';
    return dest;
}

int strcmp(const char* s1, const char* s2) {
    while (*s1 && *s1 == *s2) { s1++; s2++; }
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}

int strncmp(const char* s1, const char* s2, size_t n) {
    while (n && *s1 && *s1 == *s2) { s1++; s2++; n--; }
    if (!n) return 0;
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}

char* strchr(const char* s, int c) {
    while (*s && *s != (char)c) s++;
    return (*s == (char)c) ? (char*)s : NULL;
}

char* strrchr(const char* s, int c) {
    const char* last = NULL;
    while (*s) {
        if (*s == (char)c) last = s;
        s++;
    }
    return (*s == (char)c) ? (char*)s : (char*)last;
}

char* strstr(const char* haystack, const char* needle) {
    if (!*needle) return (char*)haystack;
    
    size_t needle_len = strlen(needle);
    while (*haystack) {
        if (!strncmp(haystack, needle, needle_len))
            return (char*)haystack;
        haystack++;
    }
    return NULL;
}

char* strdup(const char* s) {
    extern void* malloc(size_t);
    if (!s) return NULL;
    size_t len = strlen(s) + 1;
    char* dup = (char*)malloc(len);
    if (dup) memcpy(dup, s, len);
    return dup;
}

char* strndup(const char* s, size_t n) {
    extern void* malloc(size_t);
    if (!s) return NULL;
    size_t len = strnlen(s, n);
    char* dup = (char*)malloc(len + 1);
    if (dup) {
        memcpy(dup, s, len);
        dup[len] = '\0';
    }
    return dup;
}

char* strtok(char* str, const char* delim) {
    static char* saveptr = NULL;
    
    if (str) saveptr = str;
    if (!saveptr) return NULL;
    
    /* Skip leading delimiters */
    while (*saveptr && strchr(delim, *saveptr)) saveptr++;
    if (!*saveptr) return NULL;
    
    /* Find end of token */
    char* token = saveptr;
    while (*saveptr && !strchr(delim, *saveptr)) saveptr++;
    
    if (*saveptr) {
        *saveptr++ = '\0';
    }
    
    return token;
}

char* strtok_r(char* str, const char* delim, char** saveptr) {
    if (str) *saveptr = str;
    if (!*saveptr) return NULL;
    
    /* Skip leading delimiters */
    while (**saveptr && strchr(delim, **saveptr)) (*saveptr)++;
    if (!**saveptr) return NULL;
    
    /* Find end of token */
    char* token = *saveptr;
    while (**saveptr && !strchr(delim, **saveptr)) (*saveptr)++;
    
    if (**saveptr) {
        *(*saveptr)++ = '\0';
    }
    
    return token;
}

int strcasecmp(const char* s1, const char* s2) {
    unsigned char c1, c2;
    while (*s1 && *s2) {
        c1 = (unsigned char)*s1++;
        c2 = (unsigned char)*s2++;
        if (c1 >= 'A' && c1 <= 'Z') c1 += 32;
        if (c2 >= 'A' && c2 <= 'Z') c2 += 32;
        if (c1 != c2) return c1 - c2;
    }
    if (*s1) return 1;
    if (*s2) return -1;
    return 0;
}

int strncasecmp(const char* s1, const char* s2, size_t n) {
    unsigned char c1, c2;
    while (n-- && *s1 && *s2) {
        c1 = (unsigned char)*s1++;
        c2 = (unsigned char)*s2++;
        if (c1 >= 'A' && c1 <= 'Z') c1 += 32;
        if (c2 >= 'A' && c2 <= 'Z') c2 += 32;
        if (c1 != c2) return c1 - c2;
    }
    if (n && *s1) return 1;
    if (n && *s2) return -1;
    return 0;
}

char* strerror(int errnum) {
    static char buf[32];
    /* Simple implementation - just return error number */
    buf[0] = 'E'; buf[1] = 'R'; buf[2] = 'R'; buf[3] = 'O'; buf[4] = 'R';
    buf[5] = '-'; buf[6] = '\0';
    /* Could convert errnum to string here */
    return buf;
}
