#ifndef _STDDEF_H
#define _STDDEF_H

/* Standard type definitions */
typedef long int ptrdiff_t;
typedef unsigned long int size_t;
typedef long int ssize_t;
typedef int wchar_t;

/* Null pointer constant */
#define NULL ((void *)0)

/* Offset of member in structure */
#define offsetof(type, member) ((size_t) &((type *)0)->member)

#endif
