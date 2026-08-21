/* libc/src/stdio/stdio.c */
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

/* Global  */
static FILE _stdin  = {0, NULL, 0, 0, 0};
static FILE _stdout = {1, NULL, 0, 0, 0};
static FILE _stderr = {2, NULL, 0, 0, 0};

FILE* stdin  = &_stdin;
FILE* stdout = &_stdout;
FILE* stderr = &_stderr;

FILE* fopen(const char* filename, const char* mode) {
    FILE* f = (FILE*)malloc(sizeof(FILE));
    if (!f) return NULL;
    
    int flags = 0;
    if (mode[0] == 'r') flags = O_RDONLY;
    else if (mode[0] == 'w') flags = O_WRONLY | O_CREAT | O_TRUNC;
    else if (mode[0] == 'a') flags = O_WRONLY | O_CREAT | O_APPEND;
    
    int fd = open(filename, flags, 0666);
    if (fd < 0) {
        free(f);
        return NULL;
    }
    
    f->fd = fd;
    f->buffer = NULL;
    f->bufsize = 0;
    f->pos = 0;
    f->flags = 0;
    
    return f;
}

FILE* fdopen(int fd, const char* mode) {
    FILE* f = (FILE*)malloc(sizeof(FILE));
    if (!f) return NULL;
    
    f->fd = fd;
    f->buffer = NULL;
    f->bufsize = 0;
    f->pos = 0;
    f->flags = 0;
    
    return f;
}

int fclose(FILE* stream) {
    if (!stream) return -1;
    int ret = close(stream->fd);
    if (stream->buffer) free(stream->buffer);
    if (stream != stdin && stream != stdout && stream != stderr)
        free(stream);
    return ret;
}

int fgetc(FILE* stream) {
    unsigned char c;
    if (read(stream->fd, &c, 1) <= 0)
        return EOF;
    return c;
}

int fputc(int c, FILE* stream) {
    unsigned char ch = (unsigned char)c;
    if (write(stream->fd, &ch, 1) <= 0)
        return EOF;
    return c;
}

int fread(void* ptr, size_t size, size_t nmemb, FILE* stream) {
    size_t total = size * nmemb;
    ssize_t nread = read(stream->fd, ptr, total);
    if (nread <= 0) return 0;
    return (int)(nread / size);
}

int fwrite(const void* ptr, size_t size, size_t nmemb, FILE* stream) {
    size_t total = size * nmemb;
    ssize_t nwritten = write(stream->fd, ptr, total);
    if (nwritten <= 0) return 0;
    return (int)(nwritten / size);
}

char* fgets(char* s, int size, FILE* stream) {
    int i = 0;
    int c;
    while (i < size - 1) {
        c = fgetc(stream);
        if (c == EOF) {
            if (i == 0) return NULL;
            break;
        }
        s[i++] = (char)c;
        if (c == '\n') break;
    }
    s[i] = 0;
    return s;
}

int fputs(const char* s, FILE* stream) {
    return (int)write(stream->fd, s, strlen(s));
}

int fseek(FILE* stream, long offset, int whence) {
    return (int)lseek(stream->fd, (off_t)offset, whence);
}

long ftell(FILE* stream) {
    return (long)lseek(stream->fd, 0, SEEK_CUR);
}

void rewind(FILE* stream) {
    fseek(stream, 0, SEEK_SET);
}

int feof(FILE* stream) {
    return (stream->flags & 1) ? 1 : 0;
}

int ferror(FILE* stream) {
    return (stream->flags & 2) ? 1 : 0;
}

int fileno(FILE* stream) {
    return stream->fd;
}

void clearerr(FILE* stream) {
    stream->flags = 0;
}

/* Simple printf implementation */
static int do_printf(FILE* stream, const char* format, va_list ap) {
    int count = 0;
    while (*format) {
        if (*format == '%' && *(format + 1)) {
            format++;
            switch (*format) {
                case 'd': case 'i': {
                    int val = va_arg(ap, int);
                    char buf[32];
                    int pos = 0;
                    int n = val;
                    if (n < 0) {
                        fputc('-', stream);
                        n = -n;
                        count++;
                    }
                    if (n == 0) {
                        buf[pos++] = '0';
                    } else {
                        char tmp[32];
                        int tp = 0;
                        while (n > 0) {
                            tmp[tp++] = '0' + (n % 10);
                            n /= 10;
                        }
                        while (tp--) buf[pos++] = tmp[tp];
                    }
                    for (int i = 0; i < pos; i++) {
                        fputc(buf[i], stream);
                        count++;
                    }
                    break;
                }
                case 's': {
                    const char* s = va_arg(ap, const char*);
                    while (*s) {
                        fputc(*s, stream);
                        s++;
                        count++;
                    }
                    break;
                }
                case 'c': {
                    int c = va_arg(ap, int);
                    fputc(c, stream);
                    count++;
                    break;
                }
                case 'x': case 'X': {
                    unsigned int val = va_arg(ap, unsigned int);
                    char buf[16];
                    const char* hex = (*format == 'x') ? "0123456789abcdef" : "0123456789ABCDEF";
                    int pos = 0;
                    if (val == 0) {
                        buf[pos++] = '0';
                    } else {
                        unsigned int v = val;
                        char tmp[16];
                        int tp = 0;
                        while (v > 0) {
                            tmp[tp++] = hex[v & 0xF];
                            v >>= 4;
                        }
                        while (tp--) buf[pos++] = tmp[tp];
                    }
                    for (int i = 0; i < pos; i++) {
                        fputc(buf[i], stream);
                        count++;
                    }
                    break;
                }
                case '%':
                    fputc('%', stream);
                    count++;
                    break;
                default:
                    fputc('%', stream);
                    fputc(*format, stream);
                    count += 2;
                    break;
            }
            format++;
        } else {
            fputc(*format, stream);
            count++;
            format++;
        }
    }
    return count;
}

int printf(const char* format, ...) {
    va_list ap;
    va_start(ap, format);
    int ret = vprintf(format, ap);
    va_end(ap);
    return ret;
}

int vprintf(const char* format, va_list ap) {
    return vfprintf(stdout, format, ap);
}

int fprintf(FILE* stream, const char* format, ...) {
    va_list ap;
    va_start(ap, format);
    int ret = vfprintf(stream, format, ap);
    va_end(ap);
    return ret;
}

int vfprintf(FILE* stream, const char* format, va_list ap) {
    return do_printf(stream, format, ap);
}

int sprintf(char* str, const char* format, ...) {
    va_list ap;
    va_start(ap, format);
    int ret = vsprintf(str, format, ap);
    va_end(ap);
    return ret;
}

int vsprintf(char* str, const char* format, va_list ap) {
    /* Simplified - just copy format for now */
    strcpy(str, format);
    return (int)strlen(format);
}

int snprintf(char* str, size_t size, const char* format, ...) {
    va_list ap;
    va_start(ap, format);
    int ret = vsnprintf(str, size, format, ap);
    va_end(ap);
    return ret;
}

int vsnprintf(char* str, size_t size, const char* format, va_list ap) {
    strncpy(str, format, size - 1);
    str[size - 1] = 0;
    return (int)strlen(str);
}
// epstein fuck nigger bumbumbumubum jews jews