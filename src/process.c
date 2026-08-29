#include "process.h"
#include "memory.h"
#include "vga.h"
#include "kuzulib/fs/vfs.h"
#include "interrupts.h"
#include "irq.h"

#define MAX_PROCESSES 256
#define DEFAULT_STACK_SIZE 16384  // 16KB default stack
// okay date is 05.06.2026 and we have a issue at tcc i will updaate this if i remember to also its 02:51 rn and got school tomorrow also AP exam in 10 days
// Forward declarations for assembly functions
extern void context_save(struct cpu_context* ctx);
extern void context_restore(struct cpu_context* ctx);

// Basit strcpy fonksiyonu
void strcpy(char* dest, const char* src) {
    while (*src) {
        *dest = *src;
        dest++;
        src++;
    }
    *dest = 0;
}

// Basit strcmp fonksiyonu
int strcmp(const char* s1, const char* s2) {
    while (*s1 && *s2) {
        if ((unsigned char)*s1 != (unsigned char)*s2) return (unsigned char)*s1 - (unsigned char)*s2;
        s1++;
        s2++;
    }
    return (unsigned char)*s1 - (unsigned char)*s2;
}

// Basit strncmp fonksiyonu
int strncmp(const char* s1, const char* s2, int n) {
    for (int i = 0; i < n; i++) {
        unsigned char a = (unsigned char)s1[i];
        unsigned char b = (unsigned char)s2[i];
        if (a != b) return a - b;
        if (a == 0) return 0;
    }
    return 0;
}

int strlen(const char* str) {
    int len = 0;
    while (str && str[len] != '\0') len++;
    return len;
}

// Process list (doubly linked for easier removal)
struct process* process_list = 0;
struct process* current_process = 0;
struct process* shell_process = 0;
struct process* init_process = 0;
uint32_t next_pid = 1;
static struct process* last_background_process = 0;

void process_init() {
    // Initialize empty process list
    process_list = 0;
    current_process = 0;
    shell_process = 0;
    init_process = 0;
    next_pid = 1;
}

// Initialize CPU context for a new process
void init_process_context(struct process* proc, void* entry_point, uint64_t stack_base, uint64_t stack_size) {
    // Clear context
    for (int i = 0; i < sizeof(struct cpu_context); i++) {
        ((char*)&proc->context)[i] = 0;
    }
    
  // ts does NOT work twin
  // TODO: FIX THE MOTHERFUCKING STACK
    uint64_t* stack = (uint64_t*)(stack_base + stack_size);
    stack--;  // Allocate space for return address
    *stack = (uint64_t)entry_point;  // Put entry point as return address
    
    // Set initial stack pointer (points to where we put the return address)
    proc->context.rsp = (uint64_t)stack;
    proc->context.rbp = proc->context.rsp;
    proc->context.userrsp = proc->context.rsp;
    
    // Set entry point (not used directly, but good for debugging)
    proc->context.rip = (uint64_t)entry_point;
    
    // Set segment registers (kernel mode initially - we'll switch to user mode when needed)
    proc->context.cs = 0x08;  // Kernel code segment
    proc->context.ds = 0x10;  // Kernel data segment
    proc->context.es = 0x10;
    proc->context.fs = 0x10;
    proc->context.gs = 0x10;
    proc->context.ss = 0x10;  // Kernel stack segment
    
    // Set initial flags (interrupts enabled)
    proc->context.rflags = 0x202;  // IF=1, reserved bit=1
}

// Suspend current process at a valid resume point (for Ctrl+Z support)
// This saves the context such that it can be restored to resume execution
// at the point where this function was called (via process_suspend_self_yield())
void process_suspend_self() {
    extern void context_switch(struct process* from, struct process* to);
    extern struct process* shell_process;
    
    if (!current_process || !shell_process) {
        return;
    }
    
    // Mark shell as ready and switch to it
    // The shell will resume at its yield point
    shell_process->state = PROCESS_READY;
    
    // Save current context and switch to shell
    // When this process is continued, it will resume from here
    context_switch(current_process, shell_process);
    
    // When we return here, the process is being resumed (fg/bg)
    // Clear background flag if resuming as foreground
    if (current_process->is_background && current_process->state == PROCESS_RUNNING) {
        current_process->is_background = 0;
        current_process->state = PROCESS_RUNNING;
    }
}

uint32_t process_create(char* name, void* entry_point, uint64_t stack_size) {
    if (next_pid >= MAX_PROCESSES) {
        return 0; // Process limit reached
    }
    
    if (stack_size == 0) {
        stack_size = DEFAULT_STACK_SIZE;
    }
    
    // Allocate process structure
    struct process* new_process = (struct process*)kmalloc(sizeof(struct process));
    if (!new_process) {
        return 0;
    }
    
    // Allocate stack
    uint64_t stack_base = (uint64_t)kmalloc(stack_size);
    if (!stack_base) {
        kfree(new_process);
        return 0;
    }
    
    // Initialize process
    new_process->pid = next_pid++;
    new_process->state = PROCESS_READY;
    new_process->stack = stack_base;
    new_process->stack_size = stack_size;
    new_process->is_shell = 0;
    new_process->is_background = 0;
    new_process->needs_irq_restore = 0;
    new_process->irq_ret_slot = 0;
    new_process->exit_code = 0;
    new_process->elf_filename_ptr = 0;  // No filename initially
    new_process->cmd_args[0] = '\0';    // No command args initially
    strcpy(new_process->name, name);
    
    // Initialize heap (no pre-allocation, allocate on-demand via brk)
    new_process->heap_start = 0;  // Will be set on first brk call
    new_process->heap_end = 0;
    new_process->heap_max = 0x7FFFFFFF;  // Allow up to ~2GB heap
    
    // Initialize CPU context
    init_process_context(new_process, entry_point, stack_base, stack_size);
    
    // Add to process list (at the END for round-robin fairness)
    new_process->next = 0;
    if (process_list) {
        struct process* p = process_list;
        while (p->next) p = p->next;  // Find tail
        p->next = new_process;
        new_process->prev = p;
    } else {
        process_list = new_process;
        new_process->prev = 0;
    }
    for (int i = 0; i < 64; i++) new_process->fds[i] = 0;
    return new_process->pid;
}

// Create shell process
uint32_t process_create_shell() {
    extern void shell_run();
    
    uint32_t pid = process_create("shell", (void*)shell_run, DEFAULT_STACK_SIZE);
    if (pid) {
        shell_process = process_find(pid);
        if (shell_process) {
            shell_process->is_shell = 1;
        }
    }
    return pid;
}

// Find process by PID
struct process* process_find(uint32_t pid) {
    struct process* p = process_list;
    while (p) {
        if (p->pid == pid) {
            return p;
        }
        p = p->next;
    }
    return 0;
}

// Context switching helper - save current context
void context_save_wrapper(struct cpu_context* ctx) {
    context_save(ctx);
}

// Context switching helper - restore context
void context_restore_wrapper(struct cpu_context* ctx) {
    context_restore(ctx);
}

// Save a process interrupted by an IRQ/syscall frame (Ctrl+Z backgrounding).
void process_save_irq_context(struct process* proc, struct regs* r) {
    proc->context.rax = r->rax;
    proc->context.rbx = r->rbx;
    proc->context.rcx = r->rcx;
    proc->context.rdx = r->rdx;
    proc->context.rsi = r->rsi;
    proc->context.rdi = r->rdi;
    proc->context.rbp = r->rbp;
    proc->context.r8 = r->r8;
    proc->context.r9 = r->r9;
    proc->context.r10 = r->r10;
    proc->context.r11 = r->r11;
    proc->context.r12 = r->r12;
    proc->context.r13 = r->r13;
    proc->context.r14 = r->r14;
    proc->context.r15 = r->r15;
    proc->context.rip = r->rip;
    proc->context.rflags = r->rflags;
    proc->context.cs = r->cs ? r->cs : 0x08;
    proc->context.ss = r->ss ? r->ss : 0x10;
    proc->context.ds = r->ds ? r->ds : 0x10;

    // Always use the dedicated slot — never touch the live ELF stack.
    proc->irq_ret_slot = r->rip;
    proc->context.rsp = (uint64_t)&proc->irq_ret_slot;
    proc->context.userrsp = r->rsp;
    proc->needs_irq_restore = 0;
}

int process_regs_on_process_stack(struct process* proc, struct regs* r) {
    if (!proc || !r || !proc->stack || !proc->stack_size) {
        return 0;
    }
    uint64_t sp = r->rsp;
    return sp >= proc->stack && sp < proc->stack + proc->stack_size;
}

// Rebuild a ret frame on the process's real stack before context_restore().
void process_prepare_context_restore(struct process* proc) {
    if (!proc || !proc->context.rip) {
        return;
    }

    // Prefer the real stack captured at interrupt/switch time (userrsp points
    // just above the return-address slot, same layout context_save uses).
    if (proc->stack && proc->stack_size &&
        proc->context.userrsp > proc->stack &&
        proc->context.userrsp <= proc->stack + proc->stack_size) {
        uint64_t frame = proc->context.userrsp - 8;
        if (frame >= proc->stack) {
            *(uint64_t*)frame = proc->context.rip;
            proc->context.rsp = frame;
            proc->needs_irq_restore = 0;
            return;
        }
    }

    // Fallback: slot already chosen (context_save or irq_ret_slot).
    if (proc->context.rsp) {
        *(uint64_t*)proc->context.rsp = proc->context.rip;
    }
    proc->needs_irq_restore = 0;
}

// Patch an IRQ/syscall iretq frame to resume a process saved via context_save().
void process_iretq_to_process(struct regs* r, struct process* proc) {
    if (!r || !proc || !proc->context.rsp || !proc->context.rip) {
        return;
    }

    r->rip = proc->context.rip;
    r->rsp = proc->context.userrsp ? proc->context.userrsp : proc->context.rsp + 8;
    r->cs = proc->context.cs ? proc->context.cs : 0x08;
    r->ss = proc->context.ss ? proc->context.ss : 0x10;
    r->rflags = proc->context.rflags ? proc->context.rflags : 0x202;

    r->rax = proc->context.rax;
    r->rbx = proc->context.rbx;
    r->rcx = proc->context.rcx;
    r->rdx = proc->context.rdx;
    r->rsi = proc->context.rsi;
    r->rdi = proc->context.rdi;
    r->rbp = proc->context.rbp;
    r->r8 = proc->context.r8;
    r->r9 = proc->context.r9;
    r->r10 = proc->context.r10;
    r->r11 = proc->context.r11;
    r->r12 = proc->context.r12;
    r->r13 = proc->context.r13;
    r->r14 = proc->context.r14;
    r->r15 = proc->context.r15;
    r->ds = proc->context.ds ? proc->context.ds : 0x10;

    proc->state = PROCESS_RUNNING;
    current_process = proc;
}

// Return foreground shell after a background CPU slice expires.
void process_end_background_slice(struct regs* r) {
    if (!r || !bg_slice_end || !current_process || !current_process->is_background) {
        return;
    }
    if (!shell_process || shell_process->state != PROCESS_READY ||
        !shell_process->context.rsp || !shell_process->context.rip) {
        return;
    }

    bg_slice_end = 0;
    bg_slice_active = 0;
    process_save_irq_context(current_process, r);
    current_process->state = PROCESS_BACKGROUND;
    shell_process->state = PROCESS_RUNNING;
    current_process = shell_process;
    process_iretq_to_process(r, shell_process);
}

// Switch context between processes
void context_switch(struct process* from, struct process* to) {
    if (!from || !to || from == to) {
        return;
    }
    
    // Save current process context
    context_save(&from->context);
    
    // Update process states
    from->state = PROCESS_READY;
    to->state = PROCESS_RUNNING;
    
    // Switch current process
    current_process = to;

    process_prepare_context_restore(to);
    // Restore new process context (this will jump to the new process)
    context_restore(&to->context);
}

// Schedule next process (round-robin)
void process_schedule() {
    if (!current_process || !process_list) {
        return;
    }
    
    // First: if there's a SERVICE process that's READY, switch to it immediately
    // Services have highest priority - they should run 24/7
    struct process* p = process_list;
    while (p) {
        if (p->is_service && p->state == PROCESS_READY && p != current_process) {
            // Switch to service immediately
            struct process* from = current_process;
            current_process = p;
            p->state = PROCESS_RUNNING;
            
            context_save(&from->context);
            if (current_process == from) {
                return;
            }
            
            process_prepare_context_restore(p);
            context_restore(&p->context);
            return;
        }
        p = p->next;
    }
    
    // SERVICE processes should NOT be preemptively switched out
    // They only yield when they block on I/O
    if (current_process->is_service && current_process->state == PROCESS_RUNNING) {
        return;  // Service keeps running
    }
    
    // If current process is STOPPED, immediately switch to shell
    if (current_process->state == PROCESS_STOPPED) {
        if (shell_process && shell_process->state == PROCESS_READY) {
            context_switch(current_process, shell_process);
            return;
        }
    }
    
    // If current process is still RUNNING, mark it schedulable again
    if (current_process->state == PROCESS_RUNNING) {
        if (current_process->is_background) {
            current_process->state = PROCESS_BACKGROUND;
        } else if (!current_process->is_service) {
            current_process->state = PROCESS_READY;
        }
    }
    
    // Find next ready process (round-robin for non-service processes)
    struct process* next = current_process->next;
    if (!next) {
        next = process_list;
    }
    
    // Look for ready/background process
    struct process* start = next;
    while (next != current_process) {
        if (next && (next->state == PROCESS_READY ||
                     next->state == PROCESS_BACKGROUND)) {
            struct process* from = current_process;
            current_process = next;
            next->state = PROCESS_RUNNING;

            context_save(&from->context);
            // When 'from' is switched back in, land here and stop
            if (current_process == from) {
                return;
            }

            process_prepare_context_restore(next);
            context_restore(&next->context);
            return;
        }
        next = next->next;
        if (!next) {
            next = process_list;
        }
        if (next == start) {
            break;  // Wrapped around, no ready processes
        }
    }
    
    // No other ready processes, stay with current if it's schedulable
    if (current_process->state == PROCESS_READY ||
        current_process->state == PROCESS_BACKGROUND ||
        current_process->state == PROCESS_SERVICE) {
        current_process->state = PROCESS_RUNNING;
    }
}

int process_has_background(void) {
    struct process* p = process_list;
    while (p) {
        if (p->state == PROCESS_BACKGROUND) {
            return 1;
        }
        p = p->next;
    }
    return 0;
}

int process_may_use_console(void) {
    if (!current_process) {
        return 1;
    }
    return !current_process->is_background;
}

static struct process* process_find_background(void) {
    if (last_background_process &&
        last_background_process->state == PROCESS_BACKGROUND &&
        last_background_process->is_background) {
        return last_background_process;
    }

    struct process* p = process_list;
    while (p) {
        if (p->state == PROCESS_BACKGROUND && p->is_background) {
            return p;
        }
        p = p->next;
    }
    return 0;
}

// Switch from foreground directly to a background process (skip init round-robin).
void process_yield_background(void) {
    struct process* bg = process_find_background();
    if (!bg || !current_process || bg == current_process) {
        return;
    }

    struct process* from = current_process;
    from->state = PROCESS_READY;
    bg->state = PROCESS_RUNNING;
    current_process = bg;

    context_save(&from->context);

    // context_save records a return into context_switch; resume readline instead.
    {
        uint64_t resume = (uint64_t)&&bg_yield_resume;
        from->context.rip = resume;
        *(uint64_t*)(from->context.rsp) = resume;
    }

    process_prepare_context_restore(bg);
    context_restore(&bg->context);

bg_yield_resume:
    return;
}

// Yield CPU to next process
void process_yield() {
    if (current_process && current_process->state == PROCESS_RUNNING) {
        process_schedule();
    }
}

// Exit current process
void process_exit_current(int exit_code) {
    // If no current process, we're in direct execution mode - just return
    if (!current_process) {
        return;
    }
    
    struct process* exiting = current_process;
    exiting->exit_code = exit_code;
    exiting->state = PROCESS_TERMINATED;

    if (exiting == last_background_process) {
        last_background_process = 0;
    }
    
    // Close all open file descriptors for this process
    extern void close_all_fds(void);
    extern void kernel_close_all_fds(void);
    close_all_fds();
    kernel_close_all_fds();
    // Don't free the stack yet - we need to switch away first
    // Save stack pointer to free later
    uint64_t stack_to_free = exiting->stack;
    exiting->stack = 0;  // Mark as freed in structure
    
    // Free the user heap allocated by SYS_BRK
    if (exiting->heap_start) {
        kfree((void*)exiting->heap_start);
        exiting->heap_start = 0;
        exiting->heap_end = 0;
        exiting->heap_max = 0;
    }

    // Free the elf_filename_ptr if it was allocated
    if (exiting->elf_filename_ptr) {
        kfree(exiting->elf_filename_ptr);
        exiting->elf_filename_ptr = 0;
    }
    
    // Remove from process list
    if (exiting->prev) {
        exiting->prev->next = exiting->next;
    } else {
        process_list = exiting->next;
    }
    if (exiting->next) {
        exiting->next->prev = exiting->prev;
    }
    
    // Save pointer to exiting process (we'll free it after switching)
    struct process* to_free = exiting;
    
    // If shell process exists and is not terminated, switch to it
    if (shell_process && shell_process->state != PROCESS_TERMINATED && shell_process != exiting) {
        // Verify shell's stack is still valid
        if (!shell_process->stack || shell_process->stack_size == 0) {
            print_color("\n[Error: Shell process has invalid stack]\n", VGA_COLOR_LIGHT_RED);
            if (stack_to_free) {
                kfree((void*)stack_to_free);
            }
            kfree(to_free);
            while (1) {
                __asm__ volatile("cli; hlt");
            }
        }
        
        // Re-initialize shell context (restart shell from beginning)
        // The shell's context might be stale, so we'll set it up fresh
        extern void shell_run();
        
        // Re-initialize shell context to point to shell_run entry point
        init_process_context(shell_process, (void*)shell_run, shell_process->stack, shell_process->stack_size);
        
        shell_process->state = PROCESS_RUNNING;
        current_process = shell_process;
        
        // Free the exiting process's stack now (before switching)
        if (stack_to_free) {
            kfree((void*)stack_to_free);
        }
        
        // Free the process structure
        kfree(to_free);
        
        // CRITICAL: Re-initialize keyboard before restoring shell context
        // This ensures keyboard works after command completes
        extern void keyboard_init(void);
        keyboard_init();
        
        // CRITICAL: Ensure PIC has both timer and keyboard unmasked
        {
            uint8_t mask;
            __asm__ volatile("inb $0x21, %0" : "=a"(mask));
            mask &= ~0x03;  // Unmask IRQ0 (timer) and IRQ1 (keyboard)
            __asm__ volatile("outb %0, $0x21" : : "a"(mask));
        }
        
        // Send EOI to ensure PIC is ready
        __asm__ volatile("outb %%al, %%dx" : : "a"(0x20), "d"(0xA0));
        __asm__ volatile("outb %%al, %%dx" : : "a"(0x20), "d"(0x20));
        
        // Enable interrupts globally
        __asm__ volatile("sti");
        
        // Restore shell context - this will jump to shell_run() and restart shell
        context_restore(&shell_process->context);
        
        // Should never reach here
        print_color("\n[Error: context_restore returned unexpectedly]\n", VGA_COLOR_LIGHT_RED);
        while (1) {
            __asm__ volatile("cli; hlt");
        }
    } else {
        // Find any ready process
        struct process* next = process_list;
        while (next) {
            if (next->state == PROCESS_READY && next != exiting) {
                next->state = PROCESS_RUNNING;
                current_process = next;
                
                // Free the exiting process's stack now
                if (stack_to_free) {
                    kfree((void*)stack_to_free);
                }
                
                // Free the process structure
                kfree(to_free);
                
                // CRITICAL: Re-initialize keyboard before restoring context
                extern void keyboard_init(void);
                keyboard_init();
                
                // CRITICAL: Ensure PIC has both timer and keyboard unmasked
                {
                    uint8_t mask;
                    __asm__ volatile("inb $0x21, %0" : "=a"(mask));
                    mask &= ~0x03;  // Unmask IRQ0 (timer) and IRQ1 (keyboard)
                    __asm__ volatile("outb %0, $0x21" : : "a"(mask));
                }
                
                // Send EOI to ensure PIC is ready
                __asm__ volatile("outb %%al, %%dx" : : "a"(0x20), "d"(0xA0));
                __asm__ volatile("outb %%al, %%dx" : : "a"(0x20), "d"(0x20));
                
                // Enable interrupts globally
                __asm__ volatile("sti");
                
                // Restore next process context - this will jump and never return
                context_restore(&next->context);
                
                // Should never reach here
                print_color("\n[Error: context_restore returned unexpectedly]\n", VGA_COLOR_LIGHT_RED);
                while (1) {
                    __asm__ volatile("cli; hlt");
                }
            }
            next = next->next;
        }
        
        // No processes left - free resources and halt system
        if (stack_to_free) {
            kfree((void*)stack_to_free);
        }
        kfree(to_free);
        
        print_color("\n[No processes left - system halt]\n", VGA_COLOR_LIGHT_RED);
        while (1) {
            __asm__ volatile("cli; hlt");
        }
    }
}

// Remove process by PID (external call)
void process_remove(uint32_t pid) {
    struct process* p = process_find(pid);
    if (p && p != current_process) {
        p->state = PROCESS_TERMINATED;
        if (p->stack) {
            kfree((void*)p->stack);
        }
        
        // Remove from list
        if (p->prev) {
            p->prev->next = p->next;
        } else {
            process_list = p->next;
        }
        if (p->next) {
            p->next->prev = p->prev;
        }
        
        kfree(p);
    }
}

// Switch to shell process
void process_switch_to_shell() {
    if (shell_process && shell_process->state != PROCESS_TERMINATED) {
        if (current_process && current_process != shell_process) {
            context_switch(current_process, shell_process);
        } else if (!current_process) {
            shell_process->state = PROCESS_RUNNING;
            current_process = shell_process;
            context_restore(&shell_process->context);
        }
    }
}

// Move current foreground process to background (Ctrl+Z)
void process_background_current() {
    if (!current_process) {
        return;
    }

    if (current_process == shell_process || current_process == init_process) {
        return;
    }

    if (current_process->is_background) {
        return;
    }

    print_color("\n[Process ", VGA_COLOR_LIGHT_GREY);
    char buf[16];
    int pos = 0;
    uint32_t pid = current_process->pid;
    if (pid == 0) {
        buf[pos++] = '0';
    } else {
        char tmp[16];
        int tp = 0;
        while (pid > 0 && tp < 15) {
            tmp[tp++] = '0' + (pid % 10);
            pid /= 10;
        }
        while (tp--) buf[pos++] = tmp[tp];
    }
    buf[pos] = 0;
    print(buf);
    print(" running in background]\n");

    current_process->is_background = 1;
    current_process->state = PROCESS_BACKGROUND;
    last_background_process = current_process;

    if (shell_process && shell_process->state != PROCESS_TERMINATED) {
        shell_process->state = PROCESS_READY;
    }
}

void process_cleanup_background_by_name(const char* name) {
    if (!name || !name[0]) {
        return;
    }

    struct process* p = process_list;
    while (p) {
        struct process* next = p->next;
        if (p->state == PROCESS_BACKGROUND && p->is_background &&
            strcmp(p->name, name) == 0) {
            if (p == last_background_process) {
                last_background_process = 0;
            }
            process_kill(p->pid);
        }
        p = next;
    }
}

void process_stop_current() {
    process_background_current();
}

// Continue a stopped process by PID
int process_continue(uint32_t pid) {
    struct process* p = process_find(pid);
    if (!p) {
        return -1;  // Process not found
    }
    
    if (p->state != PROCESS_STOPPED) {
        return -2;  // Process not stopped
    }
    
    p->state = PROCESS_READY;
    return 0;
}

// Kill process by PID
int process_kill(uint32_t pid) {
    struct process* p = process_find(pid);
    if (!p) {
        return -1;  // Process not found
    }
    
    if (p == init_process) {
        return -3;  // Can't kill init
    }
    
    // Allow killing shell - init will respawn it
    
    // If killing current process, exit it properly
    if (p == current_process) {
        process_exit_current(128 + 9);  // SIGKILL
        return 0;
    }
    
    // Otherwise mark as terminated and remove
    p->state = PROCESS_TERMINATED;
    p->exit_code = 128 + 9;

    if (p == last_background_process) {
        last_background_process = 0;
    }
    
    // Free resources
    if (p->stack) {
        kfree((void*)p->stack);
    }
    if (p->heap_start) {
        kfree((void*)p->heap_start);
    }
    if (p->elf_filename_ptr) {
        kfree(p->elf_filename_ptr);
    }
    
    // Remove from list
    if (p->prev) {
        p->prev->next = p->next;
    } else {
        process_list = p->next;
    }
    if (p->next) {
        p->next->prev = p->prev;
    }
    
    // If killing shell, clear the shell_process pointer so init can respawn
    if (p == shell_process) {
        shell_process = 0;
    }
    
    kfree(p);
    return 0;
}

// Get process count
int process_count() {
    int count = 0;
    struct process* p = process_list;
    while (p) {
        count++;
        p = p->next;
    }
    return count;
}

// Get process list (for ps command)
int process_get_list(struct process** out_list, int max_count) {
    int count = 0;
    struct process* p = process_list;
    while (p && count < max_count) {
        if (p->state != PROCESS_TERMINATED) {
            out_list[count++] = p;
        }
        p = p->next;
    }
    return count;
} 
