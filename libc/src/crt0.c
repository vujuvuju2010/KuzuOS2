/* libc/src/crt0.c - C runtime startup for KuzuOS5 */
#include <stddef.h>
#include "../../src/z_syscalls.h"

/* External declarations */
extern int main(int argc, char* argv[]);
extern void __libc_init(void);
extern void exit(int status);

/* Global variables */
char** environ = NULL;
int errno = 0;

/* Startup function called by kernel */
void _start(void) {
    __libc_init();
    
    /* Call main with minimal args */
    int argc = 1;
    char* argv[] = { "vim", NULL };
    environ = NULL;
    
    int ret = main(argc, argv);
    exit(ret);
}

void __libc_init(void) {
    /* Initialize errno */
    errno = 0;
}
