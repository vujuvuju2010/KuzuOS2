/* KuzuOS C Library - stdio.c implementation */
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdarg.h>

/* Syscall numbers */
#define SYS_READ  3
#define SYS_WRITE 4

/* Standard file descriptors */
#define STDIN_FILENO  0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2

/* Simple syscall wrapper */
static inline int syscall3(int n, int a, int b, int c) {
    int r;
    __asm__ volatile("int $0x80" : "=a"(r) : "a"(n), "b"(a), "c"(b), "d"(c));
    return r;
}

/* FILE structure - minimal implementation */
struct _FILE {
    int fd;
    char* buffer;
    size_t buf_size;
    size_t pos;
    size_t len;
    int flags;
    int eof_flag;
    int error_flag;
};

/* Standard streams */
static FILE _stdin = {0, NULL, 0, 0, 0, 0, 0, 0};
static FILE _stdout = {1, NULL, 0, 0, 0, 0, 0, 0};
static FILE _stderr = {2, NULL, 0, 0, 0, 0, 0, 0};

FILE* stdin = &_stdin;
FILE* stdout = &_stdout;
FILE* stderr = &_stderr;

/* Mini printf implementation */
static int mini_vsnprintf(char* buffer, size_t buf_size, const char* fmt, va_list args) {
    char* p = buffer;
    char* end = buffer + buf_size - 1;
    
    while (*fmt && p < end) {
        if (*fmt != '%') {
            *p++ = *fmt++;
            continue;
        }
        fmt++;
        
        switch (*fmt) {
        case 'c': {
            char c = (char)va_arg(args, int);
            if (p < end) *p++ = c;
            break;
        }
        case 's': {
            char* s = va_arg(args, char*);
            if (!s) s = "(null)";
            while (*s && p < end) *p++ = *s++;
            break;
        }
        case 'd':
        case 'i': {
            int n = va_arg(args, int);
            char num[16];
            int i = 0;
            unsigned int un;
            
            if (n < 0) {
                if (p < end) *p++ = '-';
                un = (unsigned int)(-n);
            } else {
                un = (unsigned int)n;
            }
            
            do {
                num[i++] = '0' + (un % 10);
                un /= 10;
            } while (un && i < 15);
            
            while (i > 0 && p < end) *p++ = num[--i];
            break;
        }
        case 'u': {
            unsigned int n = va_arg(args, unsigned int);
            char num[16];
            int i = 0;
            
            do {
                num[i++] = '0' + (n % 10);
                n /= 10;
            } while (n && i < 15);
            
            while (i > 0 && p < end) *p++ = num[--i];
            break;
        }
        case 'x': {
            unsigned int n = va_arg(args, unsigned int);
            char num[16];
            int i = 0;
            static const char hex[] = "0123456789abcdef";
            
            do {
                num[i++] = hex[n & 0xF];
                n >>= 4;
            } while (n && i < 15);
            
            while (i > 0 && p < end) *p++ = num[--i];
            break;
        }
        case 'X': {
            unsigned int n = va_arg(args, unsigned int);
            char num[16];
            int i = 0;
            static const char hex[] = "0123456789ABCDEF";
            
            do {
                num[i++] = hex[n & 0xF];
                n >>= 4;
            } while (n && i < 15);
            
            while (i > 0 && p < end) *p++ = num[--i];
            break;
        }
        case 'p': {
            uintptr_t ptr = va_arg(args, uintptr_t);
            char num[20];
            int i = 0;
            static const char hex[] = "0123456789abcdef";
            
            if (p < end) *p++ = '0';
            if (p < end) *p++ = 'x';
            
            if (!ptr) {
                if (p < end) *p++ = '0';
            } else {
                do {
                    num[i++] = hex[ptr & 0xF];
                    ptr >>= 4;
                } while (ptr && i < 15);
                
                while (i > 0 && p < end) *p++ = num[--i];
            }
            break;
        }
        case '%':
            if (p < end) *p++ = '%';
            break;
        default:
            if (p < end) *p++ = '%';
            if (p < end) *p++ = *fmt;
            break;
        }
        fmt++;
    }
    
    *p = '\0';
    return (int)(p - buffer);
}

int printf(const char* format, ...) {
    va_list args;
    va_start(args, format);
    
    char buffer[1024];
    int len = mini_vsnprintf(buffer, sizeof(buffer), format, args);
    
    va_end(args);
    
    if (len > 0) {
        syscall3(SYS_WRITE, STDOUT_FILENO, (int)buffer, len);
    }
    
    return len;
}

int fprintf(FILE* stream, const char* format, ...) {
    va_list args;
    va_start(args, format);
    
    char buffer[1024];
    int len = mini_vsnprintf(buffer, sizeof(buffer), format, args);
    
    va_end(args);
    
    if (len > 0 && stream) {
        syscall3(SYS_WRITE, stream->fd, (int)buffer, len);
    }
    
    return len;
}

int sprintf(char* str, const char* format, ...) {
    va_list args;
    va_start(args, format);
    int len = mini_vsnprintf(str, 0x7FFFFFFF, format, args);
    va_end(args);
    return len;
}

int snprintf(char* str, size_t size, const char* format, ...) {
    va_list args;
    va_start(args, format);
    int len = mini_vsnprintf(str, size, format, args);
    va_end(args);
    return len;
}

int vprintf(const char* format, va_list ap) {
    char buffer[1024];
    int len = mini_vsnprintf(buffer, sizeof(buffer), format, ap);
    
    if (len > 0) {
        syscall3(SYS_WRITE, STDOUT_FILENO, (int)buffer, len);
    }
    
    return len;
}

int vfprintf(FILE* stream, const char* format, va_list ap) {
    char buffer[1024];
    int len = mini_vsnprintf(buffer, sizeof(buffer), format, ap);
    
    if (len > 0 && stream) {
        syscall3(SYS_WRITE, stream->fd, (int)buffer, len);
    }
    
    return len;
}

int putchar(int c) {
    char ch = (char)c;
    syscall3(SYS_WRITE, STDOUT_FILENO, (int)&ch, 1);
    return c;
}

int fputc(int c, FILE* stream) {
    char ch = (char)c;
    int fd = stream ? stream->fd : STDOUT_FILENO;
    syscall3(SYS_WRITE, fd, (int)&ch, 1);
    return c;
}

int fputs(const char* str, FILE* stream) {
    int fd = stream ? stream->fd : STDOUT_FILENO;
    size_t len = strlen(str);
    return syscall3(SYS_WRITE, fd, (int)str, len);
}

int puts(const char* str) {
    size_t len = strlen(str);
    syscall3(SYS_WRITE, STDOUT_FILENO, (int)str, len);
    syscall3(SYS_WRITE, STDOUT_FILENO, (int)"\n", 1);
    return (int)len + 1;
}

int getchar(void) {
    char c;
    int r = syscall3(SYS_READ, STDIN_FILENO, (int)&c, 1);
    return (r > 0) ? c : EOF;
}

int fgetc(FILE* stream) {
    char c;
    int fd = stream ? stream->fd : STDIN_FILENO;
    int r = syscall3(SYS_READ, fd, (int)&c, 1);
    return (r > 0) ? c : EOF;
}

char* fgets(char* str, int n, FILE* stream) {
    int fd = stream ? stream->fd : STDIN_FILENO;
    int i = 0;
    
    while (i < n - 1) {
        int c = fgetc(stream);
        if (c == EOF) {
            if (i == 0) return NULL;
            break;
        }
        str[i++] = (char)c;
        if (c == '\n') break;
    }
    
    str[i] = '\0';
    return str;
}

/* File operations - stub implementations */
FILE* fopen(const char* filename, const char* mode) {
    /* Would need open syscall implementation */
    (void)filename;
    (void)mode;
    return NULL;
}

int fclose(FILE* stream) {
    (void)stream;
    return 0;
}

int fflush(FILE* stream) {
    (void)stream;
    return 0;
}

int fread(void* ptr, size_t size, size_t nmemb, FILE* stream) {
    int fd = stream ? stream->fd : STDIN_FILENO;
    size_t total = size * nmemb;
    int r = syscall3(SYS_READ, fd, (int)ptr, total);
    return (r > 0) ? r / (int)size : 0;
}

int fwrite(const void* ptr, size_t size, size_t nmemb, FILE* stream) {
    int fd = stream ? stream->fd : STDOUT_FILENO;
    size_t total = size * nmemb;
    int r = syscall3(SYS_WRITE, fd, (int)ptr, total);
    return (r > 0) ? r / (int)size : 0;
}

int ferror(FILE* stream) {
    (void)stream;
    return 0;
}

void clearerr(FILE* stream) {
    (void)stream;
}

int feof(FILE* stream) {
    (void)stream;
    return 0;
}
