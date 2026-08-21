#ifndef _SETJMP_H
#define _SETJMP_H

typedef int jmp_buf[64];

int setjmp(jmp_buf);
__attribute__((noreturn)) void longjmp(jmp_buf, int);

#endif
