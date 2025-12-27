#ifndef INTERRUPTS_H
#define INTERRUPTS_H

// Kendi typedef'lerimiz
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;

// Register yapısı (ISR/IRQ handler için)
// Assembly'de pushlanan sırayla: pusha + ds + int_no + err_code
struct regs {
    uint32_t edi, esi, ebp, esp;
    uint32_t ebx, edx, ecx, eax;
    uint32_t ds;
    uint32_t int_no, err_code;
};

// Interrupt descriptor table entry
struct idt_entry {
    uint16_t base_lo;
    uint16_t sel;
    uint8_t always0;
    uint8_t flags;
    uint16_t base_hi;
} __attribute__((packed));

// IDT pointer
struct idt_ptr {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

// Interrupt handler fonksiyonları
void interrupts_init();
void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags);

// PIC (Programmable Interrupt Controller) fonksiyonları
void pic_init();
void pic_send_eoi(uint8_t irq);

// Interrupt handler'lar
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