#include "interrupts.h"
#include "vga.h"
#include "syscall.h"
#include "z_utils.h"
#include "keyboard.h"
#include "irq.h"
#include "process.h"

#define IDT_ENTRIES   256
#define PIC1_COMMAND  0x20
#define PIC1_DATA     0x21
#define PIC2_COMMAND  0xA0
#define PIC2_DATA     0xA1

struct idt_entry idt[IDT_ENTRIES];
struct idt_ptr   idtp;

void pic_init(void)
{
    __asm__ volatile("outb %%al, %%dx" : : "a"(0x11), "d"(PIC1_COMMAND));
    __asm__ volatile("outb %%al, %%dx" : : "a"(0x11), "d"(PIC2_COMMAND));
    __asm__ volatile("outb %%al, %%dx" : : "a"(0x20), "d"(PIC1_DATA));
    __asm__ volatile("outb %%al, %%dx" : : "a"(0x28), "d"(PIC2_DATA));
    __asm__ volatile("outb %%al, %%dx" : : "a"(0x04), "d"(PIC1_DATA));
    __asm__ volatile("outb %%al, %%dx" : : "a"(0x02), "d"(PIC2_DATA));
    __asm__ volatile("outb %%al, %%dx" : : "a"(0x01), "d"(PIC1_DATA));
    __asm__ volatile("outb %%al, %%dx" : : "a"(0x01), "d"(PIC2_DATA));
    __asm__ volatile("outb %%al, %%dx" : : "a"(0xFF), "d"(PIC1_DATA));
    __asm__ volatile("outb %%al, %%dx" : : "a"(0xFF), "d"(PIC2_DATA));
}

void idt_set_gate(uint8_t num, uint64_t base, uint16_t sel, uint8_t ist, uint8_t flags)
{
    idt[num].base_lo  = (uint16_t)(base & 0xFFFF);
    idt[num].base_mid = (uint16_t)((base >> 16) & 0xFFFF);
    idt[num].base_hi  = (uint32_t)((base >> 32) & 0xFFFFFFFF);
    idt[num].sel      = sel;
    idt[num].ist      = ist & 0x7;
    idt[num].flags    = flags;
    idt[num].reserved = 0;
}

void interrupts_init(void)
{
    idtp.limit = (sizeof(struct idt_entry) * IDT_ENTRIES) - 1;
    idtp.base  = (uint64_t)&idt;

    for (int i = 0; i < IDT_ENTRIES; i++)
        idt_set_gate(i, 0, 0x08, 0, 0x8E);

    idt_set_gate(0,  (uint64_t)isr0,  0x08, 0, 0x8E);
    idt_set_gate(1,  (uint64_t)isr1,  0x08, 0, 0x8E);
    idt_set_gate(2,  (uint64_t)isr2,  0x08, 0, 0x8E);
    idt_set_gate(3,  (uint64_t)isr3,  0x08, 0, 0x8E);
    idt_set_gate(4,  (uint64_t)isr4,  0x08, 0, 0x8E);
    idt_set_gate(5,  (uint64_t)isr5,  0x08, 0, 0x8E);
    idt_set_gate(6,  (uint64_t)isr6,  0x08, 0, 0x8E);
    idt_set_gate(7,  (uint64_t)isr7,  0x08, 0, 0x8E);
    idt_set_gate(8,  (uint64_t)isr8,  0x08, 1, 0x8E);  // TODO: set ist=1 once you have a double-fault stack
    idt_set_gate(9,  (uint64_t)isr9,  0x08, 0, 0x8E);
    idt_set_gate(10, (uint64_t)isr10, 0x08, 0, 0x8E);
    idt_set_gate(11, (uint64_t)isr11, 0x08, 0, 0x8E);
    idt_set_gate(12, (uint64_t)isr12, 0x08, 0, 0x8E);
    idt_set_gate(13, (uint64_t)isr13, 0x08, 1, 0x8E);
    idt_set_gate(14, (uint64_t)isr14, 0x08, 1, 0x8E);
    idt_set_gate(15, (uint64_t)isr15, 0x08, 0, 0x8E);
    idt_set_gate(16, (uint64_t)isr16, 0x08, 0, 0x8E);
    idt_set_gate(17, (uint64_t)isr17, 0x08, 0, 0x8E);
    idt_set_gate(18, (uint64_t)isr18, 0x08, 1, 0x8E);
    idt_set_gate(19, (uint64_t)isr19, 0x08, 0, 0x8E);
    idt_set_gate(20, (uint64_t)isr20, 0x08, 0, 0x8E);
    idt_set_gate(21, (uint64_t)isr21, 0x08, 0, 0x8E);
    idt_set_gate(22, (uint64_t)isr22, 0x08, 0, 0x8E);
    idt_set_gate(23, (uint64_t)isr23, 0x08, 0, 0x8E);
    idt_set_gate(24, (uint64_t)isr24, 0x08, 0, 0x8E);
    idt_set_gate(25, (uint64_t)isr25, 0x08, 0, 0x8E);
    idt_set_gate(26, (uint64_t)isr26, 0x08, 0, 0x8E);
    idt_set_gate(27, (uint64_t)isr27, 0x08, 0, 0x8E);
    idt_set_gate(28, (uint64_t)isr28, 0x08, 0, 0x8E);
    idt_set_gate(29, (uint64_t)isr29, 0x08, 0, 0x8E);
    idt_set_gate(30, (uint64_t)isr30, 0x08, 0, 0x8E);
    idt_set_gate(31, (uint64_t)isr31, 0x08, 0, 0x8E);
    idt_set_gate(128,(uint64_t)isr128,0x08, 0, 0xEE); // DPL=3 syscall

    pic_init();
    __asm__ volatile("lidt %0" : : "m"(idtp));
    __asm__ volatile("sti");
}

void pic_send_eoi(uint8_t irq)
{
    if (irq >= 8)
        __asm__ volatile("outb %%al, %%dx" : : "a"(0x20), "d"(PIC2_COMMAND));
    __asm__ volatile("outb %%al, %%dx" : : "a"(0x20), "d"(PIC1_COMMAND));
}

static void print_hex64(uint64_t val)
{
    char hx[17];
    const char* hex = "0123456789ABCDEF";
    for (int i = 15; i >= 0; i--) {
        uint8_t nib = (val >> (i * 4)) & 0xF;
        hx[15 - i] = hex[nib];
    }
    hx[16] = 0;
    print(hx);
}

void isr_handler(struct regs* r)
{
    if (r->int_no != 128) {
        print_color("\n[INT ", VGA_COLOR_LIGHT_RED);
        char b[4];
        int n = (int)r->int_no;
        b[0] = '0' + (n / 10);
        b[1] = '0' + (n % 10);
        b[2] = ']';
        b[3] = 0;
        print(b);
    }

    // Handle CPU exceptions (0-31)
    if (r->int_no < 32) {
        static const char* exception_names[] = {
            "Divide by Zero", "Debug", "NMI", "Breakpoint",
            "Overflow", "Bound Range", "Invalid Opcode", "Device Not Available",
            "Double Fault", "Coprocessor Segment", "Invalid TSS", "Segment Not Present",
            "Stack Fault", "General Protection", "Page Fault", "Reserved",
            "x87 FPU Error", "Alignment Check", "Machine Check", "SIMD FP Exception",
            "Virtualization", "Reserved", "Reserved", "Reserved",
            "Reserved", "Reserved", "Reserved", "Reserved",
            "Reserved", "Reserved", "Security Exception", "Reserved"
        };

        // Special case: Invalid Opcode (INT 06) - terminate process via syscall
        if (r->int_no == 6) {
            print_color("INT 06: Invalid Opcode - terminating process\n", VGA_COLOR_YELLOW);
            print("  RIP=0x"); print_hex64(r->rip); print("\n");

            uint8_t* opcode_ptr = (uint8_t*)r->rip;
            print("  Opcode bytes: ");
            for (int i = 0; i < 8; i++) {
                uint8_t byte = opcode_ptr[i];
                char hex[3];
                hex[0] = "0123456789ABCDEF"[byte >> 4];
                hex[1] = "0123456789ABCDEF"[byte & 0xF];
                hex[2] = ' ';
                print(hex);
            }
            print("\n");

            // SYS_EXIT via int 0x80, args in rax/rbx per this kernel's own
            // ABI (NOT the Linux x86_64 syscall/rdi convention -- this
            // kernel still dispatches through isr128 using ebx/ecx/edx/etc,
            // see handle_syscall_extended call below)
            asm volatile(
                "movq $1, %%rax\n"   // SYS_EXIT
                "movq $1, %%rbx\n"   // exit code 1
                "int $0x80\n"
                :
                :
                : "rax", "rbx"
            );
            return;
        }

        print_color(" ", VGA_COLOR_LIGHT_RED);
        print_color(exception_names[r->int_no], VGA_COLOR_LIGHT_RED);
        print_color("!\n", VGA_COLOR_LIGHT_RED);

        print("INT_NO=0x"); print_hex64(r->int_no);
        print(" ERR=0x");    print_hex64(r->err_code);
        print("\n");

        print("RIP=0x");     print_hex64(r->rip);
        print(" CS=0x");     print_hex64(r->cs);
        print(" RFLAGS=0x"); print_hex64(r->rflags);
        print("\n");

        print("RBX=0x"); print_hex64(r->rbx);
        print(" RCX=0x"); print_hex64(r->rcx);
        print(" RDX=0x"); print_hex64(r->rdx);
        print("\n");

        print("RSI=0x"); print_hex64(r->rsi);
        print(" RDI=0x"); print_hex64(r->rdi);
        print(" RBP=0x"); print_hex64(r->rbp);
        print(" RSP=0x"); print_hex64(r->rsp);
        print("\n");

        if (r->cs & 3) {  // came from user mode (CPL in low 2 bits of CS)
            print("SS=0x"); print_hex64(r->ss);
            print("\n");
        }

        if (r->int_no == 14) { // Page fault
            uint64_t cr2;
            asm volatile("mov %%cr2, %0" : "=r"(cr2));
            print("CR2 (fault addr)=0x"); print_hex64(cr2);
            print(" ERR=0x");             print_hex64(r->err_code);
            print("\n");
        }

        print_color("SYSTEM HALTED\n", VGA_COLOR_LIGHT_RED);
        while (1) asm volatile("hlt");
    }

    // Syscall interrupt 0x80 (128)
    if (r->int_no == 128) {
        extern int32_t handle_syscall_extended(uint64_t syscall_num,
                                               uint64_t arg1, uint64_t arg2, uint64_t arg3,
                                               uint64_t arg4, uint64_t arg5, uint64_t arg6);

        uint64_t syscall_num = r->rax;

        int32_t result = handle_syscall_extended(
            syscall_num,
            r->rbx,   // arg1 (kept matching this kernel's own arg mapping)
            r->rcx,   // arg2
            r->rdx,   // arg3
            r->rsi,   // arg4
            r->rdi,   // arg5
            r->rbp    // arg6
        );

        r->rax = (uint64_t)result;  // return value in rax

        // Ctrl+Z may have been pressed during syscall handling (e.g. httpd net poll)
        process_handle_ctrl_z(r);

        // When a background timeslice expires, return to the waiting shell.
        process_end_background_slice(r);
        return;
    }

    // Handle IRQs (32-47)
    if (r->int_no >= 32 && r->int_no < 48) {
        uint8_t irq = (uint8_t)(r->int_no - 32);
        pic_send_eoi(irq);
    }
}
