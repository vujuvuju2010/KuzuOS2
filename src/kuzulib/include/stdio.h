/* KuzuOS C Library - stdio.h */
#ifndef _STDIO_H
#define _STDIO_H

#include <stddef.h>
#include <stdarg.h>

/* File descriptor constants */
#define STDIN_FILENO  0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2

/* EOF constant */
#define EOF (-1)

/* File operations */
typedef struct _FILE FILE;

extern FILE* stdin;
extern FILE* stdout;
extern FILE* stderr;

/* Standard I/O functions */
int printf(const char* format, ...);
int fprintf(FILE* stream, const char* format, ...);
int sprintf(char* str, const char* format, ...);
int snprintf(char* str, size_t size, const char* format, ...);
int vprintf(const char* format, va_list ap);
int vfprintf(FILE* stream, const char* format, va_list ap);
int vsprintf(char* str, const char* format, va_list ap);
int vsnprintf(char* str, size_t size, const char* format, va_list ap);

int scanf(const char* format, ...);
int fscanf(FILE* stream, const char* format, ...);
int sscanf(const char* str, const char* format, ...);

/* Character I/O */
int getchar(void);
int putchar(int c);
int fgetc(FILE* stream);
int fputc(int c, FILE* stream);
char* fgets(char* str, int n, FILE* stream);
int fputs(const char* str, FILE* stream);
int puts(const char* str);

/* File operations */
FILE* fopen(const char* filename, const char* mode);
FILE* freopen(const char* filename, const char* mode, FILE* stream);
int fclose(FILE* stream);
int fflush(FILE* stream);

/* Binary I/O */
size_t fread(void* ptr, size_t size, size_t nmemb, FILE* stream);
size_t fwrite(const void* ptr, size_t size, size_t nmemb, FILE* stream);

/* File positioning */
int fseek(FILE* stream, long offset, int whence);
long ftell(FILE* stream);
void rewind(FILE* stream);
int fgetpos(FILE* stream, long* pos);
int fsetpos(FILE* stream, long pos);

/* Error handling */
int ferror(FILE* stream);
void clearerr(FILE* stream);
int feof(FILE* stream);
int fileno(FILE* stream);

/* Buffering */
void setbuf(FILE* stream, char* buf);
int setvbuf(FILE* stream, char* buf, int mode, size_t size);

/* Temporary files */
FILE* tmpfile(void);
char* tmpnam(char* s);

/* Formatted printing to allocated string */
int asprintf(char** strp, const char* fmt, ...);
int vasprintf(char** strp, const char* fmt, va_list ap);

#endif
