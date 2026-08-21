#ifndef _GLOB_H
#define _GLOB_H

#include <stddef.h>

#define GLOB_APPEND   0x0001
#define GLOB_DOOFFS   0x0002
#define GLOB_ERR      0x0004
#define GLOB_MARK     0x0008
#define GLOB_NOCHECK  0x0010
#define GLOB_NOSORT   0x0020

typedef struct {
    size_t gl_pathc;
    char **gl_pathv;
    size_t gl_offs;
} glob_t;

int glob(const char *, int, void *, glob_t *);
void globfree(glob_t *);

#endif
