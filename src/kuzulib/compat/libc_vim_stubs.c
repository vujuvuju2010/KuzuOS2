
#include <stddef.h>
#include <stdint.h>
#include <stdarg.h>
#include <setjmp.h>
#include <signal.h>

/* Define SIZE_MAX if not defined */
#ifndef SIZE_MAX
#define SIZE_MAX ((size_t)-1)
#endif

/* FILE type for I/O operations */
typedef struct {
    int fd;
    char* buffer;
    size_t bufsize;
    size_t pos;
    int flags;
} FILE;

/* FILE streams - define early so they can be used */
FILE* stdout = (FILE*)1;
FILE* stderr = (FILE*)2;
FILE* stdin = (FILE*)0;

/* Forward declarations to avoid implicit declaration errors */
void* memcpy(void* dest, const void* src, size_t n);
void* malloc(size_t size);
size_t strlen(const char* s);

int vfprintf(FILE* stream, const char* fmt, va_list ap);
int vsnprintf(char* str, size_t size, const char* fmt, va_list ap);

/* ============================================================================
 * MEMORY ALLOCATOR - Implemented in malloc.c
 * ============================================================================ */

/* Forward declarations - implemented in malloc.c */
extern void* malloc(size_t size);
extern void free(void* ptr);
extern void* calloc(size_t nmemb, size_t size);
extern void* realloc(void* ptr, size_t size);
extern void* kmalloc(uint32_t size);
extern void kfree(void* ptr);

/* ============================================================================
 * STRING FUNCTIONS - Full POSIX-compliant implementations
 * ============================================================================ */

size_t strlen(const char* s) {
    const char* p = s;
    while (*p) p++;
    return p - s;
}

char* strcpy(char* dest, const char* src) {
    char* d = dest;
    while ((*d++ = *src++));
    return dest;
}

char* strncpy(char* dest, const char* src, size_t n) {
    size_t i;
    for (i = 0; i < n && src[i]; i++) {
        dest[i] = src[i];
    }
    for (; i < n; i++) {
        dest[i] = '\0';
    }
    return dest;
}

int strcmp(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}

int strncmp(const char* s1, const char* s2, size_t n) {
    if (n == 0) return 0;
    while (n-- > 0) {
        if (*s1 != *s2) {
            return *(unsigned char*)s1 - *(unsigned char*)s2;
        }
        if (*s1 == '\0') {
            return 0;
        }
        s1++;
        s2++;
    }
    return 0;
}

char* strchr(const char* s, int c) {
    while (*s) {
        if (*s == (char)c) return (char*)s;
        s++;
    }
    return (c == '\0') ? (char*)s : NULL;
}

char* strrchr(const char* s, int c) {
    const char* last = NULL;
    do {
        if (*s == (char)c) last = s;
    } while (*s++);
    return (char*)last;
}

char* strstr(const char* haystack, const char* needle) {
    if (!needle || !*needle) return (char*)haystack;
    
    while (*haystack) {
        const char* h = haystack;
        const char* n = needle;
        
        while (*h && *n && (*h == *n)) {
            h++;
            n++;
        }
        
        if (!*n) return (char*)haystack;
        haystack++;
    }
    
    return NULL;
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
    
    while (n-- > 0 && *src) {
        *d++ = *src++;
    }
    *d = '\0';
    return dest;
}

size_t strlcpy(char* dest, const char* src, size_t size) {
    size_t src_len = strlen(src);
    
    if (size > 0) {
        size_t copy_len = (src_len < size - 1) ? src_len : size - 1;
        memcpy(dest, src, copy_len);
        dest[copy_len] = '\0';
    }
    
    return src_len;
}

size_t strlcat(char* dest, const char* src, size_t size) {
    size_t dest_len = strlen(dest);
    size_t src_len = strlen(src);
    
    if (dest_len >= size) {
        return size + src_len;
    }
    
    size_t copy_len = size - dest_len - 1;
    if (src_len < copy_len) {
        copy_len = src_len;
    }
    
    memcpy(dest + dest_len, src, copy_len);
    dest[dest_len + copy_len] = '\0';
    
    return dest_len + src_len;
}

/* ============================================================================
 * MEMORY FUNCTIONS - Optimized implementations
 * ============================================================================ */

void* memcpy(void* dest, const void* src, size_t n) {
    unsigned char* d = (unsigned char*)dest;
    const unsigned char* s = (const unsigned char*)src;
    
    /* Word-aligned fast copy for large blocks */
    if (n >= 16 && ((uintptr_t)d % sizeof(size_t)) == ((uintptr_t)s % sizeof(size_t))) {
        /* Align to word boundary */
        while (((uintptr_t)d % sizeof(size_t)) != 0 && n > 0) {
            *d++ = *s++;
            n--;
        }
        
        /* Copy words */
        size_t* dw = (size_t*)d;
        const size_t* sw = (const size_t*)s;
        while (n >= sizeof(size_t)) {
            *dw++ = *sw++;
            n -= sizeof(size_t);
        }
        
        d = (unsigned char*)dw;
        s = (const unsigned char*)sw;
    }
    
    /* Copy remaining bytes */
    while (n-- > 0) {
        *d++ = *s++;
    }
    
    return dest;
}

void* memmove(void* dest, const void* src, size_t n) {
    unsigned char* d = (unsigned char*)dest;
    const unsigned char* s = (const unsigned char*)src;
    
    if (d == s || n == 0) {
        return dest;
    }
    
    if (d < s) {
        /* Copy forward */
        return memcpy(dest, src, n);
    } else {
        /* Copy backward to handle overlap */
        d += n;
        s += n;
        while (n-- > 0) {
            *--d = *--s;
        }
        return dest;
    }
}

void* memset(void* s, int c, size_t n) {
    unsigned char* p = (unsigned char*)s;
    unsigned char val = (unsigned char)c;
    
    /* Fast word-aligned set for large blocks */
    if (n >= 16) {
        /* Create word-sized pattern */
        size_t pattern = val;
        for (size_t i = 1; i < sizeof(size_t); i++) {
            pattern = (pattern << 8) | val;
        }
        
        /* Align to word boundary */
        while (((uintptr_t)p % sizeof(size_t)) != 0 && n > 0) {
            *p++ = val;
            n--;
        }
        
        /* Set words */
        size_t* pw = (size_t*)p;
        while (n >= sizeof(size_t)) {
            *pw++ = pattern;
            n -= sizeof(size_t);
        }
        
        p = (unsigned char*)pw;
    }
    
    /* Set remaining bytes */
    while (n-- > 0) {
        *p++ = val;
    }
    
    return s;
}

int memcmp(const void* s1, const void* s2, size_t n) {
    const unsigned char* p1 = (const unsigned char*)s1;
    const unsigned char* p2 = (const unsigned char*)s2;
    
    while (n-- > 0) {
        if (*p1 != *p2) {
            return *p1 - *p2;
        }
        p1++;
        p2++;
    }
    return 0;
}

void* memchr(const void* s, int c, size_t n) {
    const unsigned char* p = (const unsigned char*)s;
    unsigned char val = (unsigned char)c;
    
    while (n-- > 0) {
        if (*p == val) return (void*)p;
        p++;
    }
    return NULL;
}

void* memrchr(const void* s, int c, size_t n) {
    const unsigned char* p = (const unsigned char*)s + n;
    unsigned char val = (unsigned char)c;
    
    while (n-- > 0) {
        if (*--p == val) return (void*)p;
    }
    return NULL;
}

size_t strspn(const char* s, const char* accept) {
    const char* p;
    const char* a;
    size_t count = 0;
    
    for (p = s; *p; p++) {
        for (a = accept; *a; a++) {
            if (*p == *a) break;
        }
        if (!*a) break;
        count++;
    }
    return count;
}

size_t strcspn(const char* s, const char* reject) {
    const char* p;
    const char* r;
    size_t count = 0;
    
    for (p = s; *p; p++) {
        for (r = reject; *r; r++) {
            if (*p == *r) return count;
        }
        count++;
    }
    return count;
}

char* strpbrk(const char* s, const char* accept) {
    while (*s) {
        const char* a = accept;
        while (*a) {
            if (*s == *a) return (char*)s;
            a++;
        }
        s++;
    }
    return NULL;
}

char* strtok(char* str, const char* delim) {
    static char* saved = NULL;
    
    if (str) saved = str;
    if (!saved) return NULL;
    
    /* Skip leading delimiters */
    saved += strspn(saved, delim);
    if (!*saved) return NULL;
    
    /* Find end of token */
    char* token = saved;
    saved += strcspn(saved, delim);
    
    if (*saved) {
        *saved = '\0';
        saved++;
    }
    
    return token;
}

char* strdup(const char* s) {
    size_t len = strlen(s) + 1;
    char* new_str = malloc(len);
    if (new_str) {
        memcpy(new_str, s, len);
    }
    return new_str;
}

/* ============================================================================
 * I/O FUNCTIONS - Syscall wrappers and buffered I/O
 * ============================================================================ */

extern void putchar(char c);
extern int z_read(int fd, void* buf, size_t count);
extern int z_write(int fd, const void* buf, size_t count);
extern int z_open(const char* path, int flags);
extern int z_close(int fd);
extern int z_lseek(int fd, int offset, int whence);

int write(int fd, const void* buf, size_t count) {
    return z_write(fd, buf, count);
}

int read(int fd, void* buf, size_t count) {
    return z_read(fd, buf, count);
}

int open(const char* path, int flags) {
    return z_open(path, flags);
}

int close(int fd) {
    return z_close(fd);
}

int lseek(int fd, int offset, int whence) {
    return z_lseek(fd, offset, whence);
}

/* ============================================================================
 * PRINTF FAMILY - Full format string support
 * ============================================================================ */

/* Helper: Convert integer to string */
static int int_to_str(long long value, char* buf, int base, int uppercase, int* is_negative) {
    static const char digits_lower[] = "0123456789abcdef";
    static const char digits_upper[] = "0123456789ABCDEF";
    const char* digits = uppercase ? digits_upper : digits_lower;
    
    char temp[64];
    int pos = 0;
    unsigned long long uval;
    
    *is_negative = 0;
    if (value < 0 && base == 10) {
        *is_negative = 1;
        uval = -value;
    } else {
        uval = value;
    }
    
    if (uval == 0) {
        temp[pos++] = '0';
    } else {
        while (uval > 0) {
            temp[pos++] = digits[uval % base];
            uval /= base;
        }
    }
    
    /* Reverse */
    for (int i = 0; i < pos; i++) {
        buf[i] = temp[pos - 1 - i];
    }
    buf[pos] = '\0';
    
    return pos;
}

/* Helper: Convert unsigned to string */
static int uint_to_str(unsigned long long value, char* buf, int base, int uppercase) {
    static const char digits_lower[] = "0123456789abcdef";
    static const char digits_upper[] = "0123456789ABCDEF";
    const char* digits = uppercase ? digits_upper : digits_lower;
    
    char temp[64];
    int pos = 0;
    
    if (value == 0) {
        temp[pos++] = '0';
    } else {
        while (value > 0) {
            temp[pos++] = digits[value % base];
            value /= base;
        }
    }
    
    /* Reverse */
    for (int i = 0; i < pos; i++) {
        buf[i] = temp[pos - 1 - i];
    }
    buf[pos] = '\0';
    
    return pos;
}

/* vsnprintf - core formatting function */
int vsnprintf(char* str, size_t size, const char* fmt, va_list ap) {
    size_t pos = 0;
    
    #define PUT_CHAR(c) do { \
        if (pos < size - 1) str[pos] = (c); \
        pos++; \
    } while(0)
    
    #define PUT_STR(s) do { \
        const char* _s = (s); \
        while (*_s) { PUT_CHAR(*_s++); } \
    } while(0)
    
    while (*fmt) {
        if (*fmt != '%') {
            PUT_CHAR(*fmt++);
            continue;
        }
        
        fmt++; /* Skip '%' */
        
        /* Parse flags */
        int flag_minus = 0, flag_plus = 0, flag_space = 0, flag_zero = 0, flag_hash = 0;
        while (*fmt == '-' || *fmt == '+' || *fmt == ' ' || *fmt == '0' || *fmt == '#') {
            if (*fmt == '-') flag_minus = 1;
            if (*fmt == '+') flag_plus = 1;
            if (*fmt == ' ') flag_space = 1;
            if (*fmt == '0') flag_zero = 1;
            if (*fmt == '#') flag_hash = 1;
            fmt++;
        }
        
        /* Parse width */
        int width = 0;
        if (*fmt == '*') {
            width = va_arg(ap, int);
            fmt++;
        } else {
            while (*fmt >= '0' && *fmt <= '9') {
                width = width * 10 + (*fmt - '0');
                fmt++;
            }
        }
        
        /* Parse precision */
        int precision = -1;
        if (*fmt == '.') {
            fmt++;
            precision = 0;
            if (*fmt == '*') {
                precision = va_arg(ap, int);
                fmt++;
            } else {
                while (*fmt >= '0' && *fmt <= '9') {
                    precision = precision * 10 + (*fmt - '0');
                    fmt++;
                }
            }
        }
        
        /* Parse length modifier */
        int length = 0; /* 0=int, 1=long, 2=long long, 3=short, 4=char */
        if (*fmt == 'l') {
            fmt++;
            if (*fmt == 'l') {
                length = 2;
                fmt++;
            } else {
                length = 1;
            }
        } else if (*fmt == 'h') {
            fmt++;
            if (*fmt == 'h') {
                length = 4;
                fmt++;
            } else {
                length = 3;
            }
        } else if (*fmt == 'z' || *fmt == 't') {
            length = (sizeof(size_t) == sizeof(long long)) ? 2 : 1;
            fmt++;
        }
        
        /* Parse conversion specifier */
        char spec = *fmt++;
        char buf[128];
        int is_negative;
        int len;
        
        switch (spec) {
            case 'd':
            case 'i': {
                long long val;
                if (length == 2) val = va_arg(ap, long long);
                else if (length == 1) val = va_arg(ap, long);
                else val = va_arg(ap, int);
                
                len = int_to_str(val, buf, 10, 0, &is_negative);
                
                /* Handle sign */
                int sign_char = 0;
                if (is_negative) sign_char = '-';
                else if (flag_plus) sign_char = '+';
                else if (flag_space) sign_char = ' ';
                
                int total_len = len + (sign_char ? 1 : 0);
                
                /* Padding */
                if (!flag_minus && width > total_len) {
                    char pad = flag_zero ? '0' : ' ';
                    if (flag_zero && sign_char) PUT_CHAR(sign_char);
                    for (int i = 0; i < width - total_len; i++) PUT_CHAR(pad);
                    if (!flag_zero && sign_char) PUT_CHAR(sign_char);
                } else if (sign_char) {
                    PUT_CHAR(sign_char);
                }
                
                PUT_STR(buf);
                
                if (flag_minus && width > total_len) {
                    for (int i = 0; i < width - total_len; i++) PUT_CHAR(' ');
                }
                break;
            }
            
            case 'u':
            case 'o':
            case 'x':
            case 'X': {
                unsigned long long val;
                if (length == 2) val = va_arg(ap, unsigned long long);
                else if (length == 1) val = va_arg(ap, unsigned long);
                else val = va_arg(ap, unsigned int);
                
                int base = (spec == 'o') ? 8 : (spec == 'u') ? 10 : 16;
                len = uint_to_str(val, buf, base, spec == 'X');
                
                /* Prefix for # flag */
                const char* prefix = "";
                if (flag_hash && val != 0) {
                    if (spec == 'o') prefix = "0";
                    else if (spec == 'x') prefix = "0x";
                    else if (spec == 'X') prefix = "0X";
                }
                
                int prefix_len = strlen(prefix);
                int total_len = len + prefix_len;
                
                if (!flag_minus && width > total_len) {
                    char pad = flag_zero ? '0' : ' ';
                    if (flag_zero) PUT_STR(prefix);
                    for (int i = 0; i < width - total_len; i++) PUT_CHAR(pad);
                    if (!flag_zero) PUT_STR(prefix);
                } else {
                    PUT_STR(prefix);
                }
                
                PUT_STR(buf);
                
                if (flag_minus && width > total_len) {
                    for (int i = 0; i < width - total_len; i++) PUT_CHAR(' ');
                }
                break;
            }
            
            case 'c': {
                char c = (char)va_arg(ap, int);
                if (!flag_minus && width > 1) {
                    for (int i = 0; i < width - 1; i++) PUT_CHAR(' ');
                }
                PUT_CHAR(c);
                if (flag_minus && width > 1) {
                    for (int i = 0; i < width - 1; i++) PUT_CHAR(' ');
                }
                break;
            }
            
            case 's': {
                const char* s = va_arg(ap, const char*);
                if (!s) s = "(null)";
                
                int slen = strlen(s);
                if (precision >= 0 && slen > precision) slen = precision;
                
                if (!flag_minus && width > slen) {
                    for (int i = 0; i < width - slen; i++) PUT_CHAR(' ');
                }
                
                for (int i = 0; i < slen; i++) PUT_CHAR(s[i]);
                
                if (flag_minus && width > slen) {
                    for (int i = 0; i < width - slen; i++) PUT_CHAR(' ');
                }
                break;
            }
            
            case 'p': {
                void* ptr = va_arg(ap, void*);
                uintptr_t val = (uintptr_t)ptr;
                len = uint_to_str(val, buf, 16, 0);
                PUT_STR("0x");
                PUT_STR(buf);
                break;
            }
            
            case 'n': {
                int* ptr = va_arg(ap, int*);
                if (ptr) *ptr = pos;
                break;
            }
            
            case '%': {
                PUT_CHAR('%');
                break;
            }
            
            default:
                PUT_CHAR('%');
                PUT_CHAR(spec);
                break;
        }
    }
    
    if (size > 0) {
        str[pos < size ? pos : size - 1] = '\0';
    }
    
    return pos;
    
    #undef PUT_CHAR
    #undef PUT_STR
}

int snprintf(char* str, size_t size, const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int ret = vsnprintf(str, size, fmt, ap);
    va_end(ap);
    return ret;
}

int sprintf(char* str, const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int ret = vsnprintf(str, 4096, fmt, ap);  // change the 4096 to SIZE_MAX if something brakes
    va_end(ap);
    return ret;
}

int vfprintf(FILE* stream, const char* fmt, va_list ap) {
    char buf[1024];
    int len = vsnprintf(buf, sizeof(buf), fmt, ap);
    
    if (stream == stdout || stream == (FILE*)1) {
        return write(1, buf, len);
    } else if (stream == stderr || stream == (FILE*)2) {
        return write(2, buf, len);
    }
    
    return 0;
}

int fprintf(FILE* stream, const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int ret = vfprintf(stream, fmt, ap);
    va_end(ap);
    return ret;
}

int printf(const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int ret = vfprintf(stdout, fmt, ap);
    va_end(ap);
    return ret;
}

// lib ya
int atoi(const char* nptr) {
    int val = 0;
    int neg = 0;
    if (*nptr == '-') { neg = 1; nptr++; }
    while (*nptr >= '0' && *nptr <= '9') {
        val = val * 10 + (*nptr - '0');
        nptr++;
    }
    return neg ? -val : val;
}

/* FAHH */
__attribute__((noreturn)) void exit(int status) {
    extern __attribute__((noreturn)) void z_exit(int code);
    z_exit(status);
    __builtin_unreachable();
}

__attribute__((noreturn)) void abort(void) {
    exit(1);
}

/* fuck you */
char* getenv(const char* name) {
    return NULL;
}

int system(const char* command) {
    return -1;
}

// fuck the time
long time(long* t) {
    return 0;
}

/* smash the errors and fuck em */
int errno = 0;

int* __errno_location(void) {
    return &errno;
}

const char* strerror(int errnum) {
    return "error";
}

/* Environment variables - stub */
char* _environ[] = { NULL };
char** environ = _environ;

FILE* fopen(const char* filename, const char* mode) {
    write(1, "[fopen] CALLED: filename='", 27);
    if (filename) {
        int len = 0;
        while (filename[len]) len++;
        write(1, filename, len);
    } else {
        write(1, "(null)", 6);
    }
    write(1, "' mode='", 8);
    if (mode) {
        int len = 0;
        while (mode[len]) len++;
        write(1, mode, len);
    }
    write(1, "' - returning NULL\n", 19);
    return NULL;
}

FILE* fdopen(int fd, const char* mode) {
    return (FILE*)(intptr_t)(fd + 3);
}

int fclose(FILE* stream) {
    return 0;
}

int fread(void* ptr, size_t size, size_t nmemb, FILE* stream) {
    write(1, "[fread] CALLED - returning 0\n", 29);
    return 0;
}

int fwrite(const void* ptr, size_t size, size_t nmemb, FILE* stream) {
    if (stream == stdout || stream == stderr) {
        return write(1, ptr, size * nmemb);
    }
    return 0;
}

int fgetc(FILE* stream) {
    return -1;
}

int fputc(int c, FILE* stream) {
    char ch = c;
    return fwrite(&ch, 1, 1, stream);
}

int fputs(const char* s, FILE* stream) {
    return fwrite(s, 1, strlen(s), stream);
}

int fgets(char* s, int size, FILE* stream) {
    return 0;
}

int fflush(FILE* stream) {
    return 0;
}

/* Conversion functions */
long strtol(const char* nptr, char** endptr, int base) {
    long val = 0;
    int neg = 0;
    while (*nptr == ' ' || *nptr == '\t') nptr++;
    if (*nptr == '-') { neg = 1; nptr++; }
    else if (*nptr == '+') nptr++;
    
    if (base == 0) base = 10;
    while (*nptr && ((*nptr >= '0' && *nptr <= '9') || (*nptr >= 'a' && *nptr <= 'f'))) {
        int digit;
        if (*nptr >= '0' && *nptr <= '9') digit = *nptr - '0';
        else digit = *nptr - 'a' + 10;
        if (digit >= base) break;
        val = val * base + digit;
        nptr++;
    }
    if (endptr) *endptr = (char*)nptr;
    return neg ? -val : val;
}

long long strtoll(const char* nptr, char** endptr, int base) {
    return (long long)strtol(nptr, endptr, base);
}

unsigned long strtoul(const char* nptr, char** endptr, int base) {
    unsigned long val = 0;
    while (*nptr && (*nptr >= '0' && *nptr <= '9')) {
        val = val * 10 + (*nptr - '0');
        nptr++;
    }
    if (endptr) *endptr = (char*)nptr;
    return val;
}

unsigned long long strtoull(const char* nptr, char** endptr, int base) {
    return strtoul(nptr, endptr, base);
}

/* Dynamic loading - stubs */
void* dlopen(const char* filename, int flags) {
    return NULL;
}

void* dlsym(void* handle, const char* symbol) {
    return NULL;
}

int dlclose(void* handle) {
    return 0;
}

char* dlerror(void) {
    return NULL;
}

/* Realpath - stub implementation */
char* realpath(const char* path, char* resolved_path) {
    if (!path) return NULL;
    /* Simple stub - just copy the path */
    if (resolved_path) {
        strcpy(resolved_path, path);
        return resolved_path;
    }
    return NULL;
}

/* setjmp/longjmp - implemented in setjmp.S */
/* Declarations only */
extern int setjmp(jmp_buf env);
extern void longjmp(jmp_buf env, int val);

/* Signal handling - stubs */
int sigaction(int signum, const struct sigaction *act, struct sigaction *oldact) {
    return 0;
}

int sigemptyset(sigset_t *set) {
    if (set) *set = 0;
    return 0;
}

int sigaddset(sigset_t *set, int signum) {
    if (set) *set |= (1 << signum);
    return 0;
}

int sigprocmask(int how, const sigset_t *set, sigset_t *oldset) {
    return 0;
}

/* Math functions - stubs */
double ldexp(double x, int exp) {
    return x;
}

long double ldexpl(long double x, int exp) {
    return x;
}

double strtod(const char *nptr, char **endptr) {
    /* Simple stub - just return 0.0 */
    if (endptr) *endptr = (char*)nptr;
    return 0.0;
}

float strtof(const char *nptr, char **endptr) {
    return (float)strtod(nptr, endptr);
}

long double strtold(const char *nptr, char **endptr) {
    return (long double)strtod(nptr, endptr);
}

/* Semaphore stubs */
int sem_init(void *sem, int pshared, unsigned int value) {
    return 0;
}

int sem_wait(void *sem) {
    return 0;
}

int sem_post(void *sem) {
    return 0;
}

/* Time function */
long localtime(long* t) {
    return 0;
}

/* Sort function */
void qsort(void* base, size_t nmemb, size_t size, 
           int (*compar)(const void*, const void*)) {
    /* Stub - don't implement sorting */
}

/* getcwd - get current working directory */
char* getcwd(char* buf, size_t size) {
    if (buf && size > 0) {
        buf[0] = '/';
        buf[1] = '\0';
    }
    return buf;
}

/* unlink - delete file */
int unlink(const char* path) {
    return 0;
}

/* freopen - reopen file stream */
FILE* freopen(const char* filename, const char* mode, FILE* stream) {
    return stream;
}

/* mprotect - change memory protection */
int mprotect(void* addr, size_t len, int prot) {
    return 0;
}

/* Compiler builtins for 64-bit division */
unsigned long long __udivdi3(unsigned long long a, unsigned long long b) {
    if (b == 0) return 0;
    return a / b;
}

unsigned long long __umoddi3(unsigned long long a, unsigned long long b) {
    if (b == 0) return 0;
    return a % b;
}

/* also fuck you*/
void __attribute__((weak)) pthread_create(void) {}
void __attribute__((weak)) setlocale(void) {}
void __attribute__((weak)) bindtextdomain(void) {}
void __attribute__((weak)) textdomain(void) {}

/*yea nah still fuck you */
extern int main(int argc, char* argv[]);