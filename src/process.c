#include "process.h"
#include "memory.h"
#include "vga.h"
#include "kuzulib/fs/vfs.h"

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
uint32_t next_pid = 1;

void process_init() {
    // Initialize empty process list
    process_list = 0;
    current_process = 0;
    shell_process = 0;
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
    
    // Add to process list (at the front)
    new_process->next = process_list;
    new_process->prev = 0;
    if (process_list) {
        process_list->prev = new_process;
    }
    process_list = new_process;
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
    
    // Restore new process context (this will jump to the new process)
    context_restore(&to->context);
}

// Schedule next process (round-robin)
void process_schedule() {
    if (!current_process || !process_list) {
        return;
    }
    
    // Find next ready process
    struct process* next = current_process->next;
    if (!next) {
        next = process_list;
    }
    
    // Look for ready process
    struct process* start = next;
    while (next != current_process) {
        if (next && next->state == PROCESS_READY) {
            // Switch to this process
            context_switch(current_process, next);
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
    
    // No other ready processes, stay with current
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
