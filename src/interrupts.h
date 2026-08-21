#ifndef INTERRUPTS_H
#define INTERRUPTS_H

typedef unsigned char      uint8_t;
typedef unsigned short     uint16_t;
typedef unsigned int       uint32_t;
typedef unsigned long long uint64_t;

// x86_64 struct regs.
// Pushed in isr_common in this exact order (top of stack = ds, pushed last):
//   push ds  -> then r15..rax pushed in that order (see isr64.asm)
// So on-stack layout from low address to high:
//   ds, r15, r14, r13, r12, r11, r10, r9, r8,
//   rdi, rsi, rbp, rbx, rdx, rcx, rax,
//   int_no, err_code,
//   rip, cs, rflags, rsp, ss   (pushed by CPU)
//
// NOTE: unlike the old pusha-based x32 stub, there is no fake/mid-sequence
// "esp" register here — rsp field below IS the real interrupted rsp pushed
// by the CPU (same slot the old code called useresp). We keep one name only.
struct regs {
    uint64_t ds;         // [0] saved manually, mostly vestigial in long mode
    uint64_t r15;        // [1]
    uint64_t r14;        // [2]
    uint64_t r13;        // [3]
    uint64_t r12;        // [4]
    uint64_t r11;        // [5]
    uint64_t r10;        // [6]
    uint64_t r9;         // [7]
    uint64_t r8;         // [8]
    uint64_t rdi;        // [9]
    uint64_t rsi;        // [10]
    uint64_t rbp;        // [11]
    uint64_t rbx;        // [12]
    uint64_t rdx;        // [13]
    uint64_t rcx;        // [14]
    uint64_t rax;        // [15]
    uint64_t int_no;     // [16]
    uint64_t err_code;   // [17]
    uint64_t rip;        // [18] pushed by CPU
    uint64_t cs;         // [19] pushed by CPU (only low 16 bits meaningful)
    uint64_t rflags;     // [20] pushed by CPU
    uint64_t rsp;        // [21] pushed by CPU -- the REAL interrupted rsp
    uint64_t ss;         // [22] pushed by CPU (only low 16 bits meaningful)
};

struct idt_entry {
    uint16_t base_lo;     // bits 0-15 of handler address
    uint16_t sel;         // code segment selector
    uint8_t  ist;         // bits 0-2: IST index, rest reserved(0)
    uint8_t  flags;       // type/attr byte, same meaning as x32 (0x8E etc.)
    uint16_t base_mid;    // bits 16-31 of handler address
    uint32_t base_hi;     // bits 32-63 of handler address
    uint32_t reserved;    // must be zero
} __attribute__((packed));

struct idt_ptr {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed));

void interrupts_init(void);
void idt_set_gate(uint8_t num, uint64_t base, uint16_t sel, uint8_t ist, uint8_t flags);
void pic_init(void);
void pic_send_eoi(uint8_t irq);
void isr_handler(struct regs* r);

extern void isr0();
extern void isr1();
extern void isr2();
extern void isr3();
extern void isr4();
extern void isr5();
extern void isr6();
extern void isr7();
extern void isr8();
extern void isr9();
extern void isr10();
extern void isr11();
extern void isr12();
extern void isr13();
extern void isr14();
extern void isr15();
extern void isr16();
extern void isr17();
extern void isr18();
extern void isr19();
extern void isr20();
extern void isr21();
extern void isr22();
extern void isr23();
extern void isr24();
extern void isr25();
extern void isr26();
extern void isr27();
extern void isr28();
extern void isr29();
extern void isr30();
extern void isr31();
extern void isr128();

#endif
