/* libc/src/string/strcmp.c */
#include <string.h>

int strcmp(const char* s1, const char* s2) {
    while (*s1 && *s1 == *s2) {
        s1++;
        s2++;
    }
    return (unsigned char)*s1 - (unsigned char)*s2;
}

int strncmp(const char* s1, const char* s2, size_t n) {
    for (size_t i = 0; i < n; i++) {
        if (s1[i] != s2[i])
            return (unsigned char)s1[i] - (unsigned char)s2[i];
        if (!s1[i])
            return 0;
    }
    return 0;
}

int strcasecmp(const char* s1, const char* s2) {
    while (*s1 || *s2) {
        unsigned char c1 = *s1;
        unsigned char c2 = *s2;
        if (c1 >= 'A' && c1 <= 'Z') c1 += 32;
        if (c2 >= 'A' && c2 <= 'Z') c2 += 32;
        if (c1 != c2)
            return c1 - c2;
        s1++;
        s2++;
    }
    return 0;
}

int strncasecmp(const char* s1, const char* s2, size_t n) {
    for (size_t i = 0; i < n; i++) {
        unsigned char c1 = s1[i];
        unsigned char c2 = s2[i];
        if (c1 >= 'A' && c1 <= 'Z') c1 += 32;
        if (c2 >= 'A' && c2 <= 'Z') c2 += 32;
        if (c1 != c2)
            return c1 - c2;
        if (!s1[i])
            return 0;
    }
    return 0;
}
