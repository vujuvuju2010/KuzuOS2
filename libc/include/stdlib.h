/* libc/include/stdlib.h */
#ifndef _STDLIB_H
#define _STDLIB_H

#include <stddef.h>
#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1

void* malloc(size_t size);
void* calloc(size_t nmemb, size_t size);
void* realloc(void* ptr, size_t size);
void free(void* ptr);

int atoi(const char* nptr);
long atol(const char* nptr);
long long atoll(const char* nptr);
long strtol(const char* nptr, char** endptr, int base);
long long strtoll(const char* nptr, char** endptr, int base);
unsigned long strtoul(const char* nptr, char** endptr, int base);
unsigned long long strtoull(const char* nptr, char** endptr, int base);

double atof(const char* nptr);
double strtod(const char* nptr, char** endptr);

void exit(int status);
void _exit(int status);
void abort(void);
int atexit(void (*function)(void));

char* getenv(const char* name);
int system(const char* command);

int abs(int j);
long labs(long j);
long long llabs(long long j);

int rand(void);
void srand(unsigned int seed);

void qsort(void* base, size_t nmemb, size_t size, 
           int (*compar)(const void*, const void*));
char* realpath(const char* path, char* resolved_path);

#endif
