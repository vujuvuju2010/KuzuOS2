/*IRQ FOR KUZUOS2 JAHOY JAHOYYYYYY
OFC WRITTEN BY VUJUVUJU AS OF ALWAYSSSS
ALL HAIL KUZUOS2
*/

#include "interrupts.h"
#include "keyboard.h"
#include "process.h"
#include "irq.h"

extern struct process* current_process;

extern void irq0(), irq1(), irq2(), irq3(), irq4(), irq5(), irq6(), irq7();
extern void irq8(), irq9(), irq10(), irq11(), irq12(), irq13(), irq14(), irq15();

// Timer tick counter
static uint32_t timer_ticks = 0;
static int bg_timer_count = 0;
static int bg_slice_ticks = 0;
volatile int bg_schedule_pending = 0;
volatile int bg_slice_active = 0;
volatile int bg_slice_end = 0;

// Service scheduling - ensures services get regular CPU time for network polling
static int service_timer_count = 0;

static void force_schedule_after_irq(struct regs* r);

void irq_handler(struct regs* r) {
    uint8_t irq_no = r->int_no - 32;
    
    if (irq_no == 0) {
        timer_ticks++;
        
        // Keyboard is now handled by IRQ1 handler only - avoid double-reading scancodes

        // Background process scheduling
        if (process_has_background()) {
            if (++bg_timer_count >= 10) {
                bg_timer_count = 0;
                bg_schedule_pending = 1;
            }
        } else {
            bg_timer_count = 0;
            bg_schedule_pending = 0;
        }

        // Track background process CPU slice
        if (current_process && current_process->is_background && bg_slice_active) {
            if (++bg_slice_ticks >= 10) {
                bg_slice_ticks = 0;
                bg_slice_end = 1;
            }
        } else {
            bg_slice_ticks = 0;
        }

        // End background slice if time expired
        if (bg_slice_end && current_process && current_process->is_background &&
            process_regs_on_process_stack(current_process, r)) {
            process_end_background_slice(r);
        }

        // Handle Ctrl+Z
        {
            extern volatile int ctrl_z_pressed;
            extern struct process* shell_process;
            extern struct process* init_process;
            if (ctrl_z_pressed && current_process &&
                current_process != shell_process &&
                current_process != init_process) {
                process_handle_ctrl_z(r);
            }
        }
        
        // CRITICAL: If there's a background process and we're in shell,
        // yield to let background process run
        if (bg_schedule_pending && current_process == shell_process &&
            shell_process->state == PROCESS_READY) {
            bg_schedule_pending = 0;
            extern void process_yield_background(void);
            process_yield_background();
        }
        
        // Services yield voluntarily via SYS_YIELD in their main loop
        // Let normal scheduling happen - don't block here
    }
    
    // Keyboard interrupt (IRQ 1) - handle keyboard input directly
    if (irq_no == 1) {
        keyboard_handler();
    }
    
    // EOI gönder - MUST be sent for ALL IRQs, including timer (IRQ0) and keyboard (IRQ1)
    // This acknowledges the interrupt and allows the PIC to send more interrupts
    if (irq_no >= 8) {
        __asm__ volatile("outb %%al, %%dx" : : "a"(0x20), "d"(0xA0));
    }
    __asm__ volatile("outb %%al, %%dx" : : "a"(0x20), "d"(0x20));
}

// Get current timer ticks
uint32_t timer_get_ticks(void) {
    return timer_ticks;
}

// Get uptime in milliseconds (timer runs at 100Hz, so each tick is 10ms)
uint64_t get_uptime_ms(void) {
    return (uint64_t)timer_ticks * 10;
}

void process_handle_ctrl_z(struct regs* r) {
    extern volatile int ctrl_z_pressed;
    extern struct process* shell_process;
    extern struct process* init_process;
    extern void keyboard_init(void);

    if (!ctrl_z_pressed) {
        return;
    }

    if (!current_process || current_process == shell_process ||
        current_process == init_process || current_process->is_background) {
        ctrl_z_pressed = 0;
        return;
    }

    ctrl_z_pressed = 0;
    
    // Reset background slice tracking, but KEEP bg_schedule_pending for timer IRQ
    bg_slice_active = 0;
    bg_slice_end = 0;
    bg_timer_count = 0;

    keyboard_clear_modifiers();
    process_background_current();
    
    // Save the background process context from the IRQ frame
    process_save_irq_context(current_process, r);
    current_process->state = PROCESS_BACKGROUND;
    
    // CRITICAL: Re-initialize keyboard before switching to shell
    // This ensures keyboard works when shell resumes
    keyboard_init();
    
    // CRITICAL: Ensure PIC has both timer and keyboard unmasked
    {
        uint8_t mask;
        __asm__ volatile("inb $0x21, %0" : "=a"(mask));
        mask &= ~0x03;  // Unmask IRQ0 (timer) and IRQ1 (keyboard)
        __asm__ volatile("outb %0, $0x21" : : "a"(mask));
    }
    
    // CRITICAL: Enable interrupts and send EOI so keyboard/timer continue working
    __asm__ volatile("sti");
    __asm__ volatile("outb %%al, %%dx" : : "a"(0x20), "d"(0xA0));
    __asm__ volatile("outb %%al, %%dx" : : "a"(0x20), "d"(0x20));
    
    // Set shell to READY state so timer IRQ will yield to background process
    // Set bg_schedule_pending so timer IRQ knows to yield to background
    if (shell_process && shell_process->state != PROCESS_TERMINATED) {
        shell_process->state = PROCESS_READY;
        bg_schedule_pending = 1;  // Tell timer IRQ to yield to background process
        current_process = shell_process;
        context_restore(&shell_process->context);
    }
}

// Return to shell after Ctrl+Z.
// The shell's context was saved in the middle of context_switch() which is NOT
// a valid resume point. We need to re-initialize the shell context to restart
// from shell_run(), similar to what process_exit_current() does.
// CRITICAL: This function is called from IRQ handler and context_restore() never
// returns, so we MUST send EOI before calling it or keyboard interrupts will stop!
static void force_schedule_after_irq(struct regs* r) {
    extern struct process* shell_process;
    extern void context_restore(struct cpu_context* ctx);
    extern void init_process_context(struct process* proc, void* entry_point, uint64_t stack_base, uint64_t stack_size);
    extern void shell_run(void);
    extern void keyboard_clear_modifiers(void);
    extern char keyboard_buffer[];
    extern int buffer_head;
    extern int buffer_tail;

    if (!current_process || current_process->state != PROCESS_BACKGROUND) {
        return;
    }
    if (!shell_process) {
        return;
    }
    if (!shell_process->stack || !shell_process->stack_size) {
        return;
    }

    // Save background process context from IRQ frame
    process_save_irq_context(current_process, r);
    current_process->state = PROCESS_BACKGROUND;

    // Clear modifiers FIRST before any keyboard operations
    keyboard_clear_modifiers();
    
    // Flush software keyboard buffer
    buffer_head = 0;
    buffer_tail = 0;
    
    // CRITICAL: Flush hardware keyboard buffer and reset controller
    {
        uint8_t status;
        int i;
        
        // Read any pending scancodes from the keyboard output buffer
        for (i = 0; i < 32; i++) {
            __asm__ volatile("inb $0x64, %0" : "=a"(status));
            if (!(status & 0x01)) break;
            {
                uint8_t dummy;
                __asm__ volatile("inb $0x60, %0" : "=a"(dummy));
            }
        }
    }
    
    // CRITICAL: Unmask keyboard IRQ (IRQ1) and timer IRQ (IRQ0)
    {
        uint8_t mask;
        __asm__ volatile("inb $0x21, %0" : "=a"(mask));
        mask &= ~0x03;  // Unmask IRQ0 (timer) and IRQ1 (keyboard)
        __asm__ volatile("outb %0, $0x21" : : "a"(mask));
    }

    // Re-initialize shell context to restart from shell_run()
    // This is necessary because the shell's saved context is from the middle
    // of context_switch() which is not a valid resume point
    // init_process_context already sets up the stack with entry point as return address
    init_process_context(shell_process, (void*)shell_run, shell_process->stack, shell_process->stack_size);
    
    // CRITICAL: Ensure interrupts are enabled in the restored context
    // The timer interrupt (IRQ0) must fire for keyboard_poll() to be called!
    shell_process->context.rflags = 0x202;  // IF=1 (interrupts enabled), reserved bit=1
    
    shell_process->state = PROCESS_RUNNING;
    current_process = shell_process;
    
    // CRITICAL: Enable interrupts globally before restore
    __asm__ volatile("sti");
    
    // CRITICAL: Send EOI BEFORE context_restore() because it never returns!
    // Without this, the PIC won't send more interrupts and keyboard will stop working.
    // Send EOI for IRQ0 (timer) and IRQ1 (keyboard) to ensure both can fire again
    // Note: We're currently in timer IRQ context, so we MUST send EOI here
    // because context_restore() will jump to shell and never return to irq_handler()
    __asm__ volatile("outb %%al, %%dx" : : "a"(0x20), "d"(0xA0));  // EOI to slave PIC
    __asm__ volatile("outb %%al, %%dx" : : "a"(0x20), "d"(0x20));  // EOI to master PIC
    
    context_restore(&shell_process->context);
    
    // Should never return here
}


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