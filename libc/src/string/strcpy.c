/* libc/src/string/strcpy.c */
#include <string.h>

char* strcpy(char* dest, const char* src) {
    char* d = dest;
    while (*src)
        *d++ = *src++;
    *d = 0;
    return dest;
}

char* strncpy(char* dest, const char* src, size_t n) {
    size_t i;
    for (i = 0; i < n && src[i]; i++)
        dest[i] = src[i];
    for (; i < n; i++)
        dest[i] = 0;
    return dest;
}

char* strcat(char* dest, const char* src) {
    char* d = dest + strlen(dest);
    while (*src)
        *d++ = *src++;
    *d = 0;
    return dest;
}

char* strncat(char* dest, const char* src, size_t n) {
    char* d = dest + strlen(dest);
    size_t i;
    for (i = 0; i < n && src[i]; i++)
        *d++ = src[i];
    *d = 0;
    return dest;
}
