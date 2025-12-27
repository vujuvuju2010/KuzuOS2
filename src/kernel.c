// Kendi typedef'lerimiz
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;

// VGA buffer
#define VGA_BUFFER 0xB8000

// Header'ları dahil et
#include "memory.h"
#include "interrupts.h"
#include "keyboard.h"
#include "irq.h"
#include "process.h"
#include "filesystem.h"
#include "shell.h"
#include "vga.h"
#include "banner.h"
#include "syscall.h"

// Multiboot2 header (sadece multiboot için, framebuffer yok)
#define MULTIBOOT2_HEADER_MAGIC 0xE85250D6
#define MULTIBOOT2_HEADER_ARCHITECTURE 0
#define MULTIBOOT2_HEADER_LENGTH 16
#define MULTIBOOT2_HEADER_CHECKSUM -(MULTIBOOT2_HEADER_MAGIC + MULTIBOOT2_HEADER_ARCHITECTURE + MULTIBOOT2_HEADER_LENGTH)
__attribute__((section(".multiboot2.header")))
unsigned int multiboot2_header[] = {
    MULTIBOOT2_HEADER_MAGIC,
    MULTIBOOT2_HEADER_ARCHITECTURE,
    MULTIBOOT2_HEADER_LENGTH,
    MULTIBOOT2_HEADER_CHECKSUM
};

// stdint.h yoksa temel tipler
typedef unsigned long long uint64_t;
typedef unsigned int uintptr_t;
extern void gdt_init();

// Basit random fonksiyonu (seed yok, sadece görsel amaçlı)
static int fake_rand = 0;
int rand100() {
    fake_rand = (fake_rand * 1103515245 + 12345) & 0x7fffffff;
    return (fake_rand % 100);
}

void delay(int ms) {
    for (volatile int i = 0; i < ms * 7000; i++) {
        __asm__ volatile("nop");
    }
}

void kernel_main(uint32_t mb_magic, uint32_t mb_addr) {
    gdt_init();
    interrupts_init();
    vga_init(mb_magic, mb_addr);
    clear_screen();
    print_color("\n   KuzuOS 1.0 (C) 2025\n", VGA_COLOR_CYAN);
    print_color("   BIOS date: 2025-11-6\n", VGA_COLOR_LIGHT_GREY);
    print_color("   System initializing...\n\n", VGA_COLOR_LIGHT_GREY);
    delay(700);

    print("[ "); print_color("..", VGA_COLOR_YELLOW); print(" ] CPU vendor:           "); delay(350);
    print_color("GenuineIntel\n", VGA_COLOR_LIGHT_GREY);
    delay(350);

    print("[ "); print_color("..", VGA_COLOR_YELLOW); print(" ] CPU features:         "); delay(350);
    print_color("SSE2, MMX, FPU, VME\n", VGA_COLOR_LIGHT_GREY);
    delay(350);

    print("[ "); print_color("..", VGA_COLOR_YELLOW); print(" ] RAM check:            "); delay(350);
    print_color("2048 MB detected\n", VGA_COLOR_LIGHT_GREEN);
    delay(500);

    print("[ "); print_color("..", VGA_COLOR_YELLOW); print(" ] Memory manager:       "); memory_init(); delay(400);
    print_color("OK\n", VGA_COLOR_LIGHT_GREEN); delay(600);

    // Initialize filesystem (RAM overlay + tiny FS)
    fs_init();
    
    // Initialize syscall system
    syscall_init();

    print("[ "); print_color("..", VGA_COLOR_YELLOW); print(" ] PCI bus scan:         "); delay(400);
    print_color("2 devices found\n", VGA_COLOR_LIGHT_GREEN); delay(500);

    print("[ "); print_color("..", VGA_COLOR_YELLOW); print(" ] ACPI tables:          "); delay(400);
    print_color("OK\n", VGA_COLOR_LIGHT_GREEN); delay(500);

    print("[ "); print_color("..", VGA_COLOR_YELLOW); print(" ] APIC initialization:  "); delay(400);
    print_color("OK\n", VGA_COLOR_LIGHT_GREEN); delay(500);

    print("[ "); print_color("..", VGA_COLOR_YELLOW); print(" ] HPET timer:           "); delay(400);
    print_color("OK\n", VGA_COLOR_LIGHT_GREEN); delay(500);

    print("[ "); print_color("..", VGA_COLOR_YELLOW); print(" ] RTC clock:            "); delay(400);
    print_color("OK\n", VGA_COLOR_LIGHT_GREEN); delay(500);

    print("[ "); print_color("..", VGA_COLOR_YELLOW);
    if (rand100() < 85) { print_color("OK\n", VGA_COLOR_LIGHT_GREEN); } else { print_color("FAIL\n", VGA_COLOR_LIGHT_RED); } delay(900);

    print("[ "); print_color("..", VGA_COLOR_YELLOW); print(" ] Audio device:         "); delay(400);
    if (rand100() < 80) { print_color("OK\n", VGA_COLOR_LIGHT_GREEN); } else { print_color("FAIL\n", VGA_COLOR_LIGHT_RED); } delay(900);

    print("[ "); print_color("..", VGA_COLOR_YELLOW); print(" ] Video device:         "); delay(400);
    print_color("OK\n", VGA_COLOR_LIGHT_GREEN); delay(900);

    print("[ "); print_color("..", VGA_COLOR_YELLOW); print(" ] SATA controller:      "); delay(400);
    print_color("OK\n", VGA_COLOR_LIGHT_GREEN); delay(700);

    print("[ "); print_color("..", VGA_COLOR_YELLOW); print(" ] Virtualization:       "); delay(400);
    print_color("Supported\n", VGA_COLOR_LIGHT_GREEN); delay(700);

    print("[ "); print_color("..", VGA_COLOR_YELLOW); print(" ] Hypervisor:           "); delay(400);
    print_color("QEMU detected\n", VGA_COLOR_LIGHT_GREEN); delay(700);

    print("[ "); print_color("..", VGA_COLOR_YELLOW); print(" ] Bootloader:           "); delay(400);
    print_color("GRUB2\n", VGA_COLOR_LIGHT_GREEN); delay(700);

    print("[ "); print_color("..", VGA_COLOR_YELLOW); print(" ] Shell:                "); delay(400);
    print_color("OK\n", VGA_COLOR_LIGHT_GREEN); delay(900);

    print_color("\nKuzuOS successfully started!\n", VGA_COLOR_CYAN);
    print_color("Type 'help' for commands.\n\n", VGA_COLOR_LIGHT_GREY);

    // Banner loading disabled - too heavy for low-end systems
    // Uncomment below if you want to enable it later
    /*
    struct banner main_banner;
    extern uint32_t fb_width;
    extern uint32_t fb_height;
    int banner_x = (int)(fb_width / 2) - 258;
    int banner_y = 50;
    banner_init(&main_banner, banner_x, banner_y);
    // Skip banner loading - too resource intensive
    */

     uint32_t current_esp;
    __asm__ volatile("mov %%esp, %0" : "=r"(current_esp));
    extern void tss_set_kernel_stack(uint32_t, uint32_t);
    tss_set_kernel_stack(0x10, current_esp);


    shell_run();
    
    // Simple idle loop (banner updates disabled for performance)
    while (1) {
        __asm__ volatile("hlt");
    }
} 
