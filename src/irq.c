#include "interrupts.h"
#include "keyboard.h"
#include "process.h"

// IRQ handler fonksiyonları
extern void irq0(), irq1(), irq2(), irq3(), irq4(), irq5(), irq6(), irq7();
extern void irq8(), irq9(), irq10(), irq11(), irq12(), irq13(), irq14(), irq15();

// Timer tick counter
static uint32_t timer_ticks = 0;

void irq_handler(struct regs* r) {
    // IRQ numarasını al
    uint8_t irq_no = r->int_no - 32;
    
    // Timer interrupt (IRQ 0) - Preemptive multitasking
    if (irq_no == 0) {
        timer_ticks++;
        
        // Call scheduler every tick to enable preemptive multitasking
        // Only schedule if we have a current process (not during boot)
        if (current_process) {
            process_schedule();
        }
    }
    
    // Keyboard interrupt (IRQ 1)
    if (irq_no == 1) {
        keyboard_handler();
    }
    
    // EOI gönder
    if (irq_no >= 8) {
        __asm__ volatile("outb %%al, %%dx" : : "a"(0x20), "d"(0xA0));
    }
    __asm__ volatile("outb %%al, %%dx" : : "a"(0x20), "d"(0x20));
}

// Get current timer ticks
uint32_t timer_get_ticks(void) {
    return timer_ticks;
}

// Set PIT (Programmable Interval Timer) frequency
// Base frequency is 1193180 Hz
// We'll set it to 100 Hz (10ms intervals) for reasonable multitasking
void timer_set_frequency(uint32_t hz) {
    uint32_t divisor = 1193180 / hz;
    
    // Send command byte (channel 0, lobyte/hibyte, rate generator)
    __asm__ volatile("outb %%al, %%dx" : : "a"(0x36), "d"(0x43));
    
    // Send frequency divisor
    __asm__ volatile("outb %%al, %%dx" : : "a"((uint8_t)(divisor & 0xFF)), "d"(0x40));
    __asm__ volatile("outb %%al, %%dx" : : "a"((uint8_t)((divisor >> 8) & 0xFF)), "d"(0x40));
}

void irq_init() {
    // IRQ'ları IDT'ye ekle (x64: 5 args - num, base, sel, ist, flags)
    idt_set_gate(32, (uint64_t)irq0, 0x08, 0, 0x8E);
    idt_set_gate(33, (uint64_t)irq1, 0x08, 0, 0x8E);
    idt_set_gate(34, (uint64_t)irq2, 0x08, 0, 0x8E);
    idt_set_gate(35, (uint64_t)irq3, 0x08, 0, 0x8E);
    idt_set_gate(36, (uint64_t)irq4, 0x08, 0, 0x8E);
    idt_set_gate(37, (uint64_t)irq5, 0x08, 0, 0x8E);
    idt_set_gate(38, (uint64_t)irq6, 0x08, 0, 0x8E);
    idt_set_gate(39, (uint64_t)irq7, 0x08, 0, 0x8E);
    idt_set_gate(40, (uint64_t)irq8, 0x08, 0, 0x8E);
    idt_set_gate(41, (uint64_t)irq9, 0x08, 0, 0x8E);
    idt_set_gate(42, (uint64_t)irq10, 0x08, 0, 0x8E);
    idt_set_gate(43, (uint64_t)irq11, 0x08, 0, 0x8E);
    idt_set_gate(44, (uint64_t)irq12, 0x08, 0, 0x8E);
    idt_set_gate(45, (uint64_t)irq13, 0x08, 0, 0x8E);
    idt_set_gate(46, (uint64_t)irq14, 0x08, 0, 0x8E);
    idt_set_gate(47, (uint64_t)irq15, 0x08, 0, 0x8E);
    
    // Set timer frequency to 100 Hz (10ms per tick)
    timer_set_frequency(100);
} 