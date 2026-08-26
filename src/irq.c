#include "interrupts.h"
#include "keyboard.h"
#include "process.h"
#include "irq.h"

// IRQ handler fonksiyonları
extern void irq0(), irq1(), irq2(), irq3(), irq4(), irq5(), irq6(), irq7();
extern void irq8(), irq9(), irq10(), irq11(), irq12(), irq13(), irq14(), irq15();

// Timer tick counter
static uint32_t timer_ticks = 0;

static void force_schedule_after_irq(struct regs* r);

void irq_handler(struct regs* r) {
    // IRQ numarasını al
    uint8_t irq_no = r->int_no - 32;
    
    // Timer interrupt (IRQ 0) - Preemptive multitasking
    if (irq_no == 0) {
        timer_ticks++;
        
        // Poll keyboard every timer tick
        keyboard_poll();
        
        // Check Ctrl+Z flag and stop current process if set
        process_handle_ctrl_z(r);
        
        // Call scheduler every 10 ticks (100ms) - but not from interrupt!
        // Just mark that scheduling is needed
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

void process_handle_ctrl_z(struct regs* r) {
    extern volatile int ctrl_z_pressed;
    if (!ctrl_z_pressed) {
        return;
    }
    ctrl_z_pressed = 0;
    process_stop_current();
    force_schedule_after_irq(r);
}

// Force scheduler to run after IRQ by patching the interrupt return frame
static void force_schedule_after_irq(struct regs* r) {
    // When we return from this IRQ, we want to go to the scheduler
    // The scheduler will do the actual context switch
    // For now, just trigger it immediately after interrupt
    extern void process_schedule();
    extern struct process* current_process;
    
    if (current_process && current_process->state == PROCESS_STOPPED) {
        // Directly switch to shell by manipulating the saved context
        extern struct process* shell_process;
        if (shell_process && shell_process->state == PROCESS_READY) {
            // Save current (stopped) process registers
            current_process->context.rax = r->rax;
            current_process->context.rbx = r->rbx;
            current_process->context.rcx = r->rcx;
            current_process->context.rdx = r->rdx;
            current_process->context.rsi = r->rsi;
            current_process->context.rdi = r->rdi;
            current_process->context.rbp = r->rbp;
            current_process->context.rsp = r->rsp;
            current_process->context.r8 = r->r8;
            current_process->context.r9 = r->r9;
            current_process->context.r10 = r->r10;
            current_process->context.r11 = r->r11;
            current_process->context.r12 = r->r12;
            current_process->context.r13 = r->r13;
            current_process->context.r14 = r->r14;
            current_process->context.r15 = r->r15;
            current_process->context.rip = r->rip;
            current_process->context.cs = r->cs;
            current_process->context.rflags = r->rflags;
            current_process->context.userrsp = r->rsp;
            current_process->context.ss = r->ss;
            current_process->context.ds = r->ds;
            
            // Switch to shell
            shell_process->state = PROCESS_RUNNING;
            current_process = shell_process;
            
            // Modify interrupt return frame to return to shell
            r->rax = shell_process->context.rax;
            r->rbx = shell_process->context.rbx;
            r->rcx = shell_process->context.rcx;
            r->rdx = shell_process->context.rdx;
            r->rsi = shell_process->context.rsi;
            r->rdi = shell_process->context.rdi;
            r->rbp = shell_process->context.rbp;
            r->rsp = shell_process->context.rsp;
            r->r8 = shell_process->context.r8;
            r->r9 = shell_process->context.r9;
            r->r10 = shell_process->context.r10;
            r->r11 = shell_process->context.r11;
            r->r12 = shell_process->context.r12;
            r->r13 = shell_process->context.r13;
            r->r14 = shell_process->context.r14;
            r->r15 = shell_process->context.r15;
            r->rip = shell_process->context.rip;
            r->cs = shell_process->context.cs;
            r->rflags = shell_process->context.rflags;
            r->ss = shell_process->context.ss;
            r->ds = shell_process->context.ds;
        }
    }
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

    // Unmask timer (IRQ 0) and keyboard (IRQ 1) on the PIC
    uint8_t mask;
    __asm__ volatile("inb $0x21, %0" : "=a"(mask));
    mask &= ~0x03;
    __asm__ volatile("outb %0, $0x21" : : "a"(mask));
} 