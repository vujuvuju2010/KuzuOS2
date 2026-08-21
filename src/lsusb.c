/*LSUSB USER APP FOR LSUSB WRITTEN BY VUJUVUJU*/
#include "syscall.h"
#include "z_utils.h" // for printf
#include "lsusb.h" 
#define SYS_WRITE  4
#define SYS_EXIT   1

/*define the syscall1 2 and 3 for easyness and tidiness*/
static inline int syscall1(int num, int a1) {
    int ret;
    __asm__ volatile("int $0x80" : "=a"(ret) : "a"(num), "b"(a1));
    return ret;
}
static inline int syscall2(int num, int a1, int a2) {
    int ret;
    __asm__ volatile("int $0x80" : "=a"(ret) : "a"(num), "b"(a1), "c"(a2));
    return ret;
}
static inline int syscall3(int num, int a1, int a2, int a3) {
    int ret;
    __asm__ volatile("int $0x80" : "=a"(ret) : "a"(num), "b"(a1), "c"(a2), "d"(a3));
    return ret;
}


// main func
int _start() {
    usb_lsusb_entry_t devs[16];
    int count = syscall2(SYS_LSUSB, (int)devs, 16);

    if (count <= 0) {
        z_printf("no USB devices found\n");
        syscall1(SYS_EXIT, 0);
        while(1);
    }

    for (int i = 0; i < count; i++) {
        usb_lsusb_entry_t *d = &devs[i];
        z_printf("Bus ");    z_printf("%d ", d->usb_bus); // lol forgot  to cast it to int fact for myself use %d when u print any number
        z_printf(" Device "); z_printf("%d",d->addr);
        z_printf(":ID ");   z_printf("%d", d->vendor_id);
        z_printf(":");       z_printf("%d ", d->product_id);
        z_printf(" Class "); z_printf("%d ", d->class);
        z_printf(" / ");       z_printf("%d ", d->subclass);
        z_printf("\n");
    }

    syscall1(SYS_EXIT, 0);
    while(1);
}