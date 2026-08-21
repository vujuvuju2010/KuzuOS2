#include "z_syscalls.h"
#include "z_utils.h" // printf

#define SYS_GETCWD 183 

static inline int syscall2(int num, int a1, int a2) {  
    int ret;
    __asm__ volatile("int $0x80" : "=a"(ret) : "a"(num), "b"(a1), "c"(a2));
    return ret;
}


void _start(void) {
    
    char buf[256];
int len = syscall2(SYS_GETCWD, (int)buf, sizeof(buf)); // it sharts itself unless you casted the buf to a int (YES I TRIED)
if (len < 0) { 
    z_printf("pwd: failed :(\n"); // u can comment or delete this if needed but idunno
    z_exit(1);
}


z_printf("%s\n", buf);
z_exit(0); // YOU. NEED. THIS

}
