#ifndef PROCESS_H
#define PROCESS_H

struct regs;

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long long uint64_t;

// Process states
#define PROCESS_READY 0
#define PROCESS_RUNNING 1
#define PROCESS_BLOCKED 2
#define PROCESS_TERMINATED 3
#define PROCESS_STOPPED 4  // Explicitly stopped (fg/bg resume)
#define PROCESS_BACKGROUND 5  // Running in background after Ctrl+Z


struct vfs_node; // forward declare that bad bih 

// Full CPU context for context switching (x64)
struct cpu_context {
    // General purpose registers (64-bit)
    uint64_t r15;
    uint64_t r14;
    uint64_t r13;
    uint64_t r12;
    uint64_t r11;
    uint64_t r10;
    uint64_t r9;
    uint64_t r8;
    uint64_t rdi;
    uint64_t rsi;
    uint64_t rbp;
    uint64_t rsp;
    uint64_t rbx;
    uint64_t rdx;
    uint64_t rcx;
    uint64_t rax;
    
    // Segment registers (for user mode)
    uint64_t ds;
    uint64_t es;
    uint64_t fs;
    uint64_t gs;
    
    // Control registers
    uint64_t rip;
    uint64_t cs;
    uint64_t rflags;
    uint64_t userrsp;
    uint64_t ss;
};

// Process structure
struct process {
    uint32_t pid;
    uint32_t state;
    uint64_t stack;           // 64-bit address
    uint64_t stack_size;      // 64-bit size
    char name[32];
    struct process* next;
    struct process* prev;
    
    // Full CPU context
    struct cpu_context context;
    uint64_t irq_ret_slot;    // Dedicated return slot for IRQ/syscall saves
    
    // Process type flags
    uint8_t is_shell;      // Is this the shell process?
    uint8_t is_background; // Ctrl+Z moved this to background
    uint8_t needs_irq_restore; // Saved from IRQ; rebuild stack before restore
    uint8_t exit_code;     // Exit code when terminated
    
    // ELF execution data (for passing arguments to wrapper)
    void* elf_filename_ptr;  // Pointer to filename string for ELF processes
    char  cmd_args[256];     // Command-line arguments for this process (set at exec)

    // Proper argc/argv storage (avoids lossy string round-trip)
    int   exec_argc;         // Number of arguments
    char  exec_argv_data[512]; // Flat blob: null-separated argv strings
    int   exec_argv_offsets[32]; // Offsets into exec_argv_data for each argv[i]
    
    // Heap management for brk/sbrk
    uint64_t heap_start;     // Start of heap (64-bit address)
    uint64_t heap_end;       // Current end of heap (brk pointer)
    uint64_t heap_max;       // Maximum heap address allowed
    struct vfs_node *fds[64]; // a pointer of em pointers
};

// Utility fonksiyonları
void strcpy(char* dest, const char* src);
int strcmp(const char* s1, const char* s2);
int strncmp(const char* s1, const char* s2, int n);
int strlen(const char* str);

// Process management fonksiyonları
void process_init();
uint32_t process_create(char* name, void* entry_point, uint64_t stack_size);
uint32_t process_create_shell();
void process_schedule();
void process_yield();
void process_yield_background();
void process_exit_current(int exit_code);
void process_remove(uint32_t pid);
void process_switch_to_shell();
void process_stop_current();
void process_background_current();
int process_continue(uint32_t pid);
int process_kill(uint32_t pid);
int process_has_background(void);
int process_regs_on_process_stack(struct process* proc, struct regs* r);
int process_may_use_console(void);
int process_count();
int process_get_list(struct process** out_list, int max_count);
struct process* process_find(uint32_t pid);

// Context initialization
void init_process_context(struct process* proc, void* entry_point, uint64_t stack_base, uint64_t stack_size);

// Context switching
void context_save(struct cpu_context* ctx);
void context_restore(struct cpu_context* ctx);
void context_switch(struct process* from, struct process* to);
void process_save_irq_context(struct process* proc, struct regs* r);
void process_prepare_context_restore(struct process* proc);
void process_iretq_to_process(struct regs* r, struct process* proc);
void process_end_background_slice(struct regs* r);
void process_cleanup_background_by_name(const char* name);

// Current process
extern struct process* current_process;
extern struct process* shell_process;
extern struct process* init_process;
extern uint32_t next_pid;

#endif 