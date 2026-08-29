// Kernel wrapper for loader.c - replaces syscalls with kernel functions
#include "filesystem.h"
#include "memory.h"
#include "vga.h"
#include "z_elf.h"
#include "z_utils.h"
#include "z_syscalls.h"
#include "elf.h"  // For elf_load_and_run declaration
#include "process.h"

// Forward declare z_memcpy
extern void* z_memcpy(void* dest, const void* src, size_t n);

// File handle structure for kernel
typedef struct {
    char* filename;
    char* buffer;
    uint32_t size;
    uint32_t pos;
} kernel_file_t;

static kernel_file_t kernel_files[1024];  // Plenty of room on 32GB system
static int next_fd = 3;  // Start at 3 (0,1,2 are stdin,stdout,stderr)

// Helper: Close and cleanup all kernel file descriptors
void kernel_close_all_fds(void) {
    for (int i = 3; i < 1024; i++) {
        if (kernel_files[i].buffer) {
            kfree(kernel_files[i].buffer);
            kernel_files[i].buffer = 0;
        }
        kernel_files[i].filename = 0;
        kernel_files[i].size = 0;
        kernel_files[i].pos = 0;
    }
    next_fd = 3;  // Reset for next process
}

// Helper: load file contents into kernel buffer
static int load_file_into_buffer(const char* path, char** out_buffer, uint32_t* out_size) {
    int file_size = fs_get_file_size((char*)path);
    if (file_size <= 0) {
        return -1;
    }
    
    char* buffer = (char*)kmalloc((uint32_t)file_size);
    if (!buffer) {
        return -2;
    }
    
    int bytes_read = fs_read_file((char*)path, buffer, (uint32_t)file_size);
    if (bytes_read != file_size) {
        kfree(buffer);
        return -3;
    }
    
    *out_buffer = buffer;
    *out_size = (uint32_t)bytes_read;
    return 0;
}

// Helper: case-insensitive string comparison
static int strcasecmp(const char* s1, const char* s2) {
    while (*s1 && *s2) {
        char c1 = *s1;
        char c2 = *s2;
        // Case-sensitive comparison
        if (c1 != c2) return c1 - c2;
        s1++;
        s2++;
    }
    return *s1 - *s2;
}

// Helper: normalize path - remove trailing dots (keep original case)
static void normalize_path(char* dest, const char* src, int max_len) {
    int i = 0;
    while (src[i] && i < max_len - 1) {
        char c = src[i];
        // Keep original case
        dest[i] = c;
        i++;
    }
    // Remove trailing dots
    while (i > 0 && dest[i-1] == '.') i--;
    dest[i] = 0;
}

// Replace z_open with kernel filesystem
int z_open(const char *filename, int flags) {
    
    print("z_open: ");
    print(filename ? filename : "(null)");
    print("\n");
    
    if (next_fd >= 1024) {
        return -1;
    }
    
    char* loaded_buffer = 0;
    uint32_t loaded_size = 0;
    int load_result = load_file_into_buffer(filename, &loaded_buffer, &loaded_size);
    
    if (load_result != 0) {
        // Try normalized (uppercase + trimmed) path as fallback
        char normalized[256];
        normalize_path(normalized, filename, sizeof(normalized));
        load_result = load_file_into_buffer(normalized, &loaded_buffer, &loaded_size);
    }
    
    if (load_result != 0) {
        return -1;
    }
    
    kernel_file_t* f = &kernel_files[next_fd];
    f->filename = (char*)filename;
    f->buffer = loaded_buffer;
    f->size = loaded_size;
    f->pos = 0;
    
    return next_fd++;
}

// Replace z_read with kernel buffer
ssize_t z_read(int fd, void *buf, size_t count) {
    if (fd < 3 || fd >= 1024) return -1;
    kernel_file_t* f = &kernel_files[fd];
    
    if (f->pos >= f->size) return 0;
    
    size_t to_read = count;
    if (f->pos + to_read > f->size)
        to_read = f->size - f->pos;
    
    extern void* z_memcpy(void* dest, const void* src, size_t n);
    z_memcpy(buf, f->buffer + f->pos, to_read);
    f->pos += to_read;
    
    return to_read;
}

// Replace z_lseek with kernel buffer
int z_lseek(int fd, off_t offset, int whence) {
    if (fd < 3 || fd >= 1024) return -1;
    kernel_file_t* f = &kernel_files[fd];
    
    if (whence == SEEK_SET) {
        f->pos = offset;
    } else if (whence == SEEK_CUR) {
        f->pos += offset;
    } else {
        f->pos = f->size + offset;
    }
    
    if (f->pos > f->size) f->pos = f->size;
    return f->pos;
}

// Replace z_close
int z_close(int fd) {
    if (fd < 3 || fd >= 1024) return -1;
    kernel_file_t* f = &kernel_files[fd];
    
    if (f->buffer) {
        kfree(f->buffer);
        f->buffer = 0;
    }
    return 0;
}

// Replace z_mmap - just return the address (kernel space, no mapping needed)
void *z_mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset) {
    // In kernel space, we can write directly to addresses
    // Just return the requested address if MAP_FIXED
    if (flags & MAP_FIXED) {
        return addr;
    }
    // For dynamic, allocate from kernel heap
    if (flags & MAP_ANONYMOUS) {
        return kmalloc(length);
    }
    return (void*)-1;
}

// Replace z_munmap - no-op in kernel space
int z_munmap(void *addr, size_t length) {
    // In kernel space, we don't unmap
    // Could kfree if it was allocated, but for simplicity, just return success
    return 0;
}

// Replace z_mprotect - no-op in kernel space (no protection)
int z_mprotect(void *addr, size_t length, int prot) {
    return 0;
}

// Replace z_write - use kernel print
ssize_t z_write(int fd, const void *buf, size_t count) {
    // For stdout/stderr, use kernel print
    if (fd == 1 || fd == 2) {
        const char* s = (const char*)buf;
        for (size_t i = 0; i < count; i++) {
            putchar(s[i]);
        }
        return count;
    }
    return -1;
}

// Replace z_exit - restore kernel stack
__attribute__((noreturn)) void z_exit(int status) {
    // This will be handled by exit handler
    extern void elf_exit_program();
    elf_exit_program();
    __builtin_unreachable();
}

// Global variables for stack restoration
uint64_t saved_kernel_esp = 0;
uint64_t saved_kernel_ebp = 0;
uint64_t saved_kernel_esp_for_exit = 0;
uint64_t saved_kernel_ebp_for_exit = 0;
void* elf_exit_label_addr = 0;
int program_exit_requested = 0;

// Internal helper to execute ELF (called from process context)
static void elf_execute_internal(const char* filename, uint32_t stack_base, uint32_t stack_size, const char* cmd_args) {
    uint32_t user_stack_top = stack_base + stack_size;

    // Allocate string storage separately to avoid corrupting the active call stack
    #define STR_BUF_SIZE 512
    char* str_buf = (char*)kmalloc(STR_BUF_SIZE);
    if (!str_buf) {
        print_color("[ELF] str_buf alloc failed\n", VGA_COLOR_LIGHT_RED);
        extern void process_exit_current(int exit_code);
        process_exit_current(1);
        return;
    }
    // Zero it so strings are always null-terminated
    for (int i = 0; i < STR_BUF_SIZE; i++) str_buf[i] = 0;
    int str_off = 0;

    #define PUSH_STR(s) do { \
        const char* _s = (s); \
        while (*_s && str_off < STR_BUF_SIZE - 1) str_buf[str_off++] = *_s++; \
        str_buf[str_off++] = '\0'; \
    } while(0)

    // Basename of filename for argv[0]
    const char* base = filename;
    for (const char* p = filename; *p; p++)
        if (*p == '/') base = p + 1;

    char* argv_ptrs[32];
    int argc = 0;

    extern struct process* current_process;
    if (current_process && current_process->exec_argc > 0) {
        int orig_argc = current_process->exec_argc;
        for (int i = 0; i < orig_argc && argc < 31; i++) {
            argv_ptrs[argc++] = str_buf + str_off;
            const char* s = current_process->exec_argv_data + current_process->exec_argv_offsets[i];
            while (*s && str_off < STR_BUF_SIZE - 1) str_buf[str_off++] = *s++;
            str_buf[str_off++] = '\0';
        }
    } else {
        // Fallback: use cmd_args string
        argv_ptrs[argc++] = str_buf + str_off;
        PUSH_STR(base);
        argv_ptrs[argc++] = str_buf + str_off;
        PUSH_STR(filename);

        if (cmd_args) {
            int i = 0;
            while (cmd_args[i] && argc < 31) {
                while (cmd_args[i] == ' ') i++;
                if (!cmd_args[i]) break;
                argv_ptrs[argc++] = str_buf + str_off;
                while (cmd_args[i] && cmd_args[i] != ' ' && str_off < STR_BUF_SIZE - 1)
                    str_buf[str_off++] = cmd_args[i++];
                str_buf[str_off++] = '\0';
            }
        }
    }

    // Build stack from top: argc, argv[0..argc-1], NULL( argv terminator), NULL(envp terminator), AT_NULL, 0
    // x86-64 ABI stack layout for _start:
    //   [rsp+0]   = argc
    //   [rsp+8]   = argv[0]
    //   [rsp+16]  = argv[1]
    //   ...
    //   [rsp+8*argc] = NULL (argv terminator)
    //   [rsp+8*(argc+1)] = envp[0] (or NULL if no envp)
    //   ...
    //   [rsp+8*(argc+1+envc)] = NULL (envp terminator)
    //   [rsp+8*(argc+1+envc+1)] = AT_NULL
    //   [rsp+8*(argc+1+envc+2)] = 0 (auxv terminator value)
    
    uint64_t* stack = (uint64_t*)user_stack_top;
    
    // First push auxv terminator (value)
    stack -= 1; stack[0] = 0;
    // Then auxv terminator (type)
    stack -= 1; stack[0] = AT_NULL;
    // envp NULL terminator (no envp, just NULL)
    stack -= 1; stack[0] = 0;
    // argv NULL terminator
    stack -= 1; stack[0] = 0;
    // Push argv pointers in reverse order
    for (int k = argc - 1; k >= 0; k--) {
        stack -= 1;
        stack[0] = (uint64_t)argv_ptrs[k];
    }
    // Finally push argc at the top
    stack -= 1; stack[0] = (uint64_t)argc;

    unsigned long* sp = (unsigned long*)stack;
    
    __asm__ volatile("movq %%rsp, %0" : "=m"(saved_kernel_esp));
    __asm__ volatile("movq %%rbp, %0" : "=m"(saved_kernel_ebp));
    saved_kernel_esp_for_exit = saved_kernel_esp;
    saved_kernel_ebp_for_exit = saved_kernel_ebp;
    __asm__ volatile("sti");
    
    extern void z_entry(unsigned long *sp, void (*fini)(void));
    extern void z_fini(void);
    z_entry(sp, z_fini);

    kfree(str_buf);
    extern void process_exit_current(int exit_code);
    process_exit_current(0);
}

// Wrapper function that will be the entry point of a process
// Reads filename from current_process->elf_filename_ptr
static void elf_process_wrapper() {
    // Get current process to access filename and stack
    extern struct process* current_process;
    if (!current_process) {
        print_color("[ELF] Error: No current process\n", VGA_COLOR_LIGHT_RED);
        extern void process_exit_current(int exit_code);
        process_exit_current(1);
        return;
    }
    
    // Get filename from process structure
    const char* filename = (const char*)current_process->elf_filename_ptr;
    if (!filename) {
        print_color("[ELF] Error: No filename in process\n", VGA_COLOR_LIGHT_RED);
        extern void process_exit_current(int exit_code);
        process_exit_current(1);
        return;
    }
    
    // Execute ELF using the process's stack and command-line args
    elf_execute_internal(filename, current_process->stack, current_process->stack_size, current_process->cmd_args);
    
    // Should not reach here
    extern void process_exit_current(int exit_code);
    process_exit_current(0);
}

// Main entry point wrapper - creates a process for the ELF
int elf_load_and_run(const char* filename) {
    // Create a process for this ELF program
    int filename_len = 0;
    while (filename[filename_len]) filename_len++;
    
    // Allocate memory for filename copy
    char* filename_copy = (char*)kmalloc(filename_len + 1);
    if (!filename_copy) {
        extern void heap_stats(void);
        heap_stats();
        print_color("[ELF] Error: Failed to allocate memory for filename\n", VGA_COLOR_LIGHT_RED);
        return -1;
    }
    
    // Copy filename
    int i = 0;
    while (filename[i]) {
        filename_copy[i] = filename[i];
        i++;
    }
    filename_copy[i] = 0;
    
    // Extract just the name for process name
    const char* last_slash = filename_copy;
    i = 0;
    while (filename_copy[i]) {
        if (filename_copy[i] == '/') {
            last_slash = filename_copy + i + 1;
        }
        i++;
    }
    
    char process_name[32];
    i = 0;
    while (last_slash[i] && i < 31) {
        process_name[i] = last_slash[i];
        i++;
    }
    process_name[i] = 0;
    
    // Create process with wrapper function as entry point
    extern uint32_t process_create(char* name, void* entry_point, uint64_t stack_size);
    extern struct process* process_find(uint32_t pid);
    extern void context_switch(struct process* from, struct process* to);
    extern void context_restore(struct cpu_context* ctx);
    extern struct process* current_process;
    
    uint32_t pid = process_create(process_name, (void*)elf_process_wrapper, 16384);
    if (!pid) {
        print_color("[ELF] Error: Failed to create process\n", VGA_COLOR_LIGHT_RED);
        kfree(filename_copy);
        return -1;
    }
    
    // Get the process and store filename pointer in it
    struct process* proc = process_find(pid);
    if (!proc) {
        print_color("[ELF] Error: Failed to find created process\n", VGA_COLOR_LIGHT_RED);
        kfree(filename_copy);
        return -1;
    }
    
    // Store filename pointer in process structure (wrapper will read it from there)
    proc->elf_filename_ptr = filename_copy;
    
    // Store command-line arguments for this process (so z_getcmdargs can read them)
    {
        extern char kernel_cmd_args[256];
        int k = 0;
        while (k < 255 && kernel_cmd_args[k] != '\0') {
            proc->cmd_args[k] = kernel_cmd_args[k];
            k++;
        }
        proc->cmd_args[k] = '\0';
    }
    
    // Stack is already set up by init_process_context with entry point
    // We don't need to modify it since wrapper takes no arguments
    
    // Switch to the new process
    if (current_process && current_process != proc) {
        // Save current process (shell) context before switching
        proc->state = PROCESS_RUNNING;
        context_switch(current_process, proc);
        // Should never return from context_switch
    } else {
        // No current process registered - switch to new process
        // But first, check if shell is running and save its context
        extern struct process* shell_process;
        if (shell_process && shell_process->state == PROCESS_RUNNING) {
            // Shell is running but not tracked as current_process - save its context first
            // We need to save the shell's current execution state
            // For now, just mark shell as ready so we can switch back later
            shell_process->state = PROCESS_READY;
            current_process = shell_process;
            // Now switch from shell to new process
            proc->state = PROCESS_RUNNING;
            context_switch(shell_process, proc);
            // Should never return
        } else {
            // No running process to save - just switch to new process
            proc->state = PROCESS_RUNNING;
            current_process = proc;
            context_restore(&proc->context);
            // Should never return
        }
    }
    
    return 0;
}

// Get command-line args for current process (used by syscall 250)
const char* get_current_cmd_args(void) {
    extern struct process* current_process;
    extern char kernel_cmd_args[256];
    if (current_process && current_process->cmd_args[0] != '\0') {
        return current_process->cmd_args;
    }
    return kernel_cmd_args;
}

// Exit handler - handles both direct execution and process-based execution
void elf_exit_program() {
    extern struct process* current_process;
    
    // Restore kernel stack before exiting
    if (saved_kernel_esp != 0 && saved_kernel_ebp != 0) {
        __asm__ volatile("cli");
        __asm__ volatile("movq %0, %%rsp" : : "m"(saved_kernel_esp) : "memory");
        __asm__ volatile("movq %0, %%rbp" : : "m"(saved_kernel_ebp) : "memory");
        
        saved_kernel_esp = 0;
        saved_kernel_ebp = 0;
    }
    
    // If we have a current process, exit it (will switch back to shell)
    if (current_process) {
        extern void process_exit_current(int exit_code);
        process_exit_current(0);
    }
    // Otherwise, we're in direct execution mode - just return to shell
    // (The shell will continue its loop and show the prompt again)
}

// Execve - load and execute with proper argv[] array
int elf_load_and_execve(const char* filename, char* const argv[], char* const envp[]) {
    (void)envp;  // envp not yet implemented

    extern char kernel_cmd_args[256];

    // Count argc
    int argc = 0;
    if (argv) {
        while (argv[argc] != NULL && argc < 31) argc++;
    }

    // Build a flat blob of null-separated argv strings and store offsets.
    // We store this in a temporary buffer; it gets copied into proc->exec_argv_data below.
    char argv_blob[512];
    int  argv_offsets[32];
    int  blob_pos = 0;

    for (int i = 0; i < argc && blob_pos < 511; i++) {
        argv_offsets[i] = blob_pos;
        const char* s = argv[i];
        while (*s && blob_pos < 510) argv_blob[blob_pos++] = *s++;
        argv_blob[blob_pos++] = '\0';
    }

    // Also keep kernel_cmd_args in sync (used by SYS_GETCMDARGS / syscall 250)
    {
        int idx = 0;
        for (int i = 1; i < argc && idx < 255; i++) {
            const char* s = argv[i];
            while (*s && idx < 254) kernel_cmd_args[idx++] = *s++;
            if (i + 1 < argc && idx < 254) kernel_cmd_args[idx++] = ' ';
        }
        kernel_cmd_args[idx] = '\0';
    }

    // Allocate and copy filename
    int fname_len = 0;
    while (filename[fname_len]) fname_len++;
    char* filename_copy = (char*)kmalloc(fname_len + 1);
    if (!filename_copy) {
        print_color("[execve] kmalloc failed for filename\n", VGA_COLOR_LIGHT_RED);
        return -1;
    }
    for (int i = 0; i <= fname_len; i++) filename_copy[i] = filename[i];

    // Extract basename for process name
    const char* base = filename_copy;
    for (int i = 0; filename_copy[i]; i++)
        if (filename_copy[i] == '/') base = filename_copy + i + 1;

    char proc_name[32];
    int ni = 0;
    while (base[ni] && ni < 31) { proc_name[ni] = base[ni]; ni++; }
    proc_name[ni] = '\0';

    // Drop stale background jobs with the same name before launching again.
    extern void process_cleanup_background_by_name(const char* name);
    process_cleanup_background_by_name(proc_name);

    // Create process
    extern uint32_t process_create(char* name, void* entry_point, uint64_t stack_size);
    extern struct process* process_find(uint32_t pid);
    extern void context_save(struct cpu_context* ctx);
    extern void context_restore(struct cpu_context* ctx);
    extern void process_prepare_context_restore(struct process* proc);
    extern struct process* current_process;
    extern struct process* shell_process;

    uint32_t pid = process_create(proc_name, (void*)elf_process_wrapper, 16384);
    if (!pid) {
        print_color("[execve] process_create failed\n", VGA_COLOR_LIGHT_RED);
        kfree(filename_copy);
        return -1;
    }

    struct process* proc = process_find(pid);
    if (!proc) {
        print_color("[execve] process_find failed\n", VGA_COLOR_LIGHT_RED);
        kfree(filename_copy);
        return -1;
    }

    // Store filename and proper argv in process struct
    proc->elf_filename_ptr = filename_copy;
    proc->exec_argc = argc;

    // Zero out argv storage first
    for (int i = 0; i < 512; i++) proc->exec_argv_data[i] = 0;
    for (int i = 0; i < 32; i++) proc->exec_argv_offsets[i] = 0;

    // Copy argv blob and offsets
    for (int i = 0; i < blob_pos && i < 511; i++) proc->exec_argv_data[i] = argv_blob[i];
    for (int i = 0; i < argc && i < 32; i++) proc->exec_argv_offsets[i] = argv_offsets[i];
    
    // Debug: show what we stored
    z_printf("[execve] stored argc=%d for pid=%d:\n", argc, pid);
    for (int i = 0; i < argc; i++) {
        z_printf("  argv[%d] = '%s'\n", i, proc->exec_argv_data + proc->exec_argv_offsets[i]);
    }

    // Also keep cmd_args for legacy SYS_GETCMDARGS
    {
        int k = 0;
        while (k < 255 && kernel_cmd_args[k]) { proc->cmd_args[k] = kernel_cmd_args[k]; k++; }
        proc->cmd_args[k] = '\0';
    }

    // Switch to the new foreground process.  Save the caller's context with a
    // resume label so Ctrl+Z can iretq back here instead of rebooting the shell.
    struct process* from = current_process;
    if (!from) {
        proc->state = PROCESS_RUNNING;
        current_process = proc;
        context_restore(&proc->context);
        return 0;
    }

    from->state = PROCESS_READY;
    proc->state = PROCESS_RUNNING;
    current_process = proc;

    context_save(&from->context);
    {
        uint64_t resume = (uint64_t)&&exec_foreground_return;
        from->context.rip = resume;
        *(uint64_t*)(from->context.rsp) = resume;
    }

    process_prepare_context_restore(proc);
    context_restore(&proc->context);

exec_foreground_return:
    return 0;
}

// Load ELF and create a background process (for services)
// Unlike elf_load_and_execve, this doesn't switch to the new process
int elf_load_and_create_background(const char* filename, char* const argv[], uint32_t* pid_out) {
    // Count argc
    int argc = 0;
    if (argv) {
        while (argv[argc] != NULL && argc < 31) argc++;
    }

    // Build a flat blob of null-separated argv strings and store offsets
    char argv_blob[512];
    int  argv_offsets[32];
    int  blob_pos = 0;

    for (int i = 0; i < argc && blob_pos < 511; i++) {
        argv_offsets[i] = blob_pos;
        const char* s = argv[i];
        while (*s && blob_pos < 510) argv_blob[blob_pos++] = *s++;
        argv_blob[blob_pos++] = '\0';
    }

    // Allocate and copy filename
    int fname_len = 0;
    while (filename[fname_len]) fname_len++;
    char* filename_copy = (char*)kmalloc(fname_len + 1);
    if (!filename_copy) {
        print_color("[elf_load_background] kmalloc failed for filename\n", VGA_COLOR_LIGHT_RED);
        return -1;
    }
    for (int i = 0; i <= fname_len; i++) filename_copy[i] = filename[i];

    // Extract basename for process name
    const char* base = filename_copy;
    for (int i = 0; filename_copy[i]; i++)
        if (filename_copy[i] == '/') base = filename_copy + i + 1;

    char proc_name[32];
    int ni = 0;
    while (base[ni] && ni < 31) { proc_name[ni] = base[ni]; ni++; }
    proc_name[ni] = '\0';

    // Create process
    extern uint32_t process_create(char* name, void* entry_point, uint64_t stack_size);
    extern struct process* process_find(uint32_t pid);

    uint32_t pid = process_create(proc_name, (void*)elf_process_wrapper, 16384);
    if (!pid) {
        print_color("[elf_load_background] process_create failed\n", VGA_COLOR_LIGHT_RED);
        kfree(filename_copy);
        return -1;
    }

    struct process* proc = process_find(pid);
    if (!proc) {
        print_color("[elf_load_background] process_find failed\n", VGA_COLOR_LIGHT_RED);
        kfree(filename_copy);
        return -1;
    }

    // Store filename and proper argv in process struct
    proc->elf_filename_ptr = filename_copy;
    proc->exec_argc = argc;

    // Zero out argv storage first
    for (int i = 0; i < 512; i++) proc->exec_argv_data[i] = 0;
    for (int i = 0; i < 32; i++) proc->exec_argv_offsets[i] = 0;

    // Copy argv blob and offsets
    for (int i = 0; i < blob_pos && i < 511; i++) proc->exec_argv_data[i] = argv_blob[i];
    for (int i = 0; i < argc && i < 32; i++) proc->exec_argv_offsets[i] = argv_offsets[i];

    // Mark process as SERVICE - set to READY so scheduler will pick it up
    proc->state = PROCESS_READY;
    proc->is_service = 1;

    // Return the PID
    *pid_out = pid;
    
    print_color("[elf_load_background] created background process '", VGA_COLOR_LIGHT_GREEN);
    print_color(proc_name, VGA_COLOR_LIGHT_GREEN);
    print_color("' with PID ", VGA_COLOR_LIGHT_GREEN);
    char pid_buf[16];
    int n = pid;
    int pos = 0;
    if (n == 0) pid_buf[pos++] = '0';
    else {
        while (n > 0) {
            pid_buf[pos++] = '0' + (n % 10);
            n /= 10;
        }
    }
    for (int j = pos - 1; j >= 0; j--) {
        putchar(pid_buf[j]);
    }
    print_color("\n", VGA_COLOR_LIGHT_GREEN);

    return 0;
}

// Fault recovery handler
void elf_fault_recovery() {
    // Restore kernel stack immediately
    if (saved_kernel_esp_for_exit != 0 && saved_kernel_ebp_for_exit != 0) {
        __asm__ volatile("cli");
        __asm__ volatile("movq %0, %%rsp" : : "m"(saved_kernel_esp_for_exit) : "memory");
        __asm__ volatile("movq %0, %%rbp" : : "m"(saved_kernel_ebp_for_exit) : "memory");
        
        saved_kernel_esp_for_exit = 0;
        saved_kernel_ebp_for_exit = 0;
        
        // Jump to cleanup if available
        if (elf_exit_label_addr != 0) {
            void* exit_label = elf_exit_label_addr;
            elf_exit_label_addr = 0;
            goto *exit_label;
        }
    }
}

