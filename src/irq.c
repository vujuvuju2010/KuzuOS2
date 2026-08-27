#include "interrupts.h"
#include "keyboard.h"
#include "process.h"
#include "irq.h"

extern struct process* current_process;

// IRQ handler fonksiyonları
extern void irq0(), irq1(), irq2(), irq3(), irq4(), irq5(), irq6(), irq7();
extern void irq8(), irq9(), irq10(), irq11(), irq12(), irq13(), irq14(), irq15();

// Timer tick counter
static uint32_t timer_ticks = 0;
static int bg_timer_count = 0;
static int bg_slice_ticks = 0;
volatile int bg_schedule_pending = 0;
volatile int bg_slice_active = 0;
volatile int bg_slice_end = 0;

static void force_schedule_after_irq(struct regs* r);

void irq_handler(struct regs* r) {
    // IRQ numarasını al
    uint8_t irq_no = r->int_no - 32;
    
    // Timer interrupt (IRQ 0) - Preemptive multitasking
    if (irq_no == 0) {
        timer_ticks++;
        
        // Poll keyboard every timer tick
        keyboard_poll();

        // Offer one background timeslice ~every 100ms when something is backgrounded
        if (process_has_background()) {
            if (++bg_timer_count >= 10) {
                bg_timer_count = 0;
                bg_schedule_pending = 1;
            }
        } else {
            bg_timer_count = 0;
            bg_schedule_pending = 0;
        }

        // End an active background slice ~100ms after it started
        if (current_process && current_process->is_background && bg_slice_active) {
            if (++bg_slice_ticks >= 10) {
                bg_slice_ticks = 0;
                bg_slice_end = 1;
            }
        } else {
            bg_slice_ticks = 0;
        }

        // Preempt background process back to shell when its slice expires
        if (bg_slice_end && current_process && current_process->is_background &&
            process_regs_on_process_stack(current_process, r)) {
            process_end_background_slice(r);
        }

        // Ctrl+Z: handle on timer IRQ when interrupted on the process stack.
        // If inside a syscall (kernel stack), leave ctrl_z_pressed for int 0x80 return.
        {
            extern volatile int ctrl_z_pressed;
            extern struct process* shell_process;
            extern struct process* init_process;
            if (ctrl_z_pressed && current_process &&
                current_process != shell_process &&
                current_process != init_process &&
                process_regs_on_process_stack(current_process, r)) {
                process_handle_ctrl_z(r);
            }
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

void process_handle_ctrl_z(struct regs* r) {
    extern volatile int ctrl_z_pressed;
    extern struct process* shell_process;
    extern struct process* init_process;

    if (!ctrl_z_pressed) {
        return;
    }

    if (!current_process || current_process == shell_process ||
        current_process == init_process || current_process->is_background) {
        ctrl_z_pressed = 0;
        return;
    }

    ctrl_z_pressed = 0;
    bg_slice_active = 0;
    bg_slice_end = 0;
    bg_schedule_pending = 0;
    bg_timer_count = 0;

    keyboard_clear_modifiers();
    process_background_current();
    force_schedule_after_irq(r);
}

// Return to shell after Ctrl+Z — restore shell's saved exec-wait context.
static void force_schedule_after_irq(struct regs* r) {
    extern struct process* shell_process;

    if (!current_process || current_process->state != PROCESS_BACKGROUND) {
        return;
    }
    if (!shell_process || shell_process->state != PROCESS_READY) {
        return;
    }
    if (!shell_process->context.rsp || !shell_process->context.rip) {
        return;
    }

    process_save_irq_context(current_process, r);
    current_process->state = PROCESS_BACKGROUND;

    shell_process->state = PROCESS_RUNNING;
    current_process = shell_process;
    process_iretq_to_process(r, shell_process);
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