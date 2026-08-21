/* libc/src/stdlib/strtol.c */
#include <stdlib.h>
#include <ctype.h>

static int is_digit(char c) {
    return c >= '0' && c <= '9';
}

static int digit_value(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'z') return c - 'a' + 10;
    if (c >= 'A' && c <= 'Z') return c - 'A' + 10;
    return -1;
}

long strtol(const char* nptr, char** endptr, int base) {
    const char* s = nptr;
    long result = 0;
    int negative = 0;
    
    /* Skip whitespace */
    while (*s == ' ' || *s == '\t' || *s == '\n') s++;
    
    /* Handle sign */
    if (*s == '-') { negative = 1; s++; }
    else if (*s == '+') s++;
    
    /* Auto-detect base */
    if (base == 0) {
        if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) {
            base = 16;
            s += 2;
        } else if (s[0] == '0') {
            base = 8;
            s++;
        } else {
            base = 10;
        }
    } else if (base == 16 && s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) {
        s += 2;
    }
    
    /* Parse digits */
    while (*s) {
        int digit = digit_value(*s);
        if (digit < 0 || digit >= base) break;
        result = result * base + digit;
        s++;
    }
    
    if (endptr) *endptr = (char*)s;
    return negative ? -result : result;
}

long long strtoll(const char* nptr, char** endptr, int base) {
    return (long long)strtol(nptr, endptr, base);
}

unsigned long strtoul(const char* nptr, char** endptr, int base) {
    return (unsigned long)strtol(nptr, endptr, base);
}

unsigned long long strtoull(const char* nptr, char** endptr, int base) {
    return (unsigned long long)strtol(nptr, endptr, base);
}

int atoi(const char* nptr) {
    return (int)strtol(nptr, NULL, 10);
}

long atol(const char* nptr) {
    return strtol(nptr, NULL, 10);
}

long long atoll(const char* nptr) {
    return strtoll(nptr, NULL, 10);
}

double atof(const char* nptr) {
    return strtod(nptr, NULL);
}

double strtod(const char* nptr, char** endptr) {
    /* Simplified strtod - just parse integer part for now */
    long ival = strtol(nptr, endptr, 10);
    return (double)ival;
}
