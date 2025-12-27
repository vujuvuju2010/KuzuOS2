// Kernel wrapper for loader.c - replaces syscalls with kernel functions
#include "filesystem.h"
#include "memory.h"
#include "vga.h"
#include "z_elf.h"
#include "z_utils.h"
#include "z_syscalls.h"
#include "elf.h"  // For elf_load_and_run declaration

// Forward declare z_memcpy
extern void* z_memcpy(void* dest, const void* src, size_t n);

// File handle structure for kernel
typedef struct {
    char* filename;
    char* buffer;
    uint32_t size;
    uint32_t pos;
} kernel_file_t;

static kernel_file_t kernel_files[16];
static int next_fd = 3;  // Start at 3 (0,1,2 are stdin,stdout,stderr)

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
        // Convert to uppercase for comparison
        if (c1 >= 'a' && c1 <= 'z') c1 = c1 - 'a' + 'A';
        if (c2 >= 'a' && c2 <= 'z') c2 = c2 - 'a' + 'A';
        if (c1 != c2) return c1 - c2;
        s1++;
        s2++;
    }
    return *s1 - *s2;
}

// Helper: normalize path - uppercase and remove trailing dots
static void normalize_path(char* dest, const char* src, int max_len) {
    int i = 0;
    while (src[i] && i < max_len - 1) {
        char c = src[i];
        // Convert to uppercase
        if (c >= 'a' && c <= 'z') c = c - 'a' + 'A';
        dest[i] = c;
        i++;
    }
    // Remove trailing dots
    while (i > 0 && dest[i-1] == '.') i--;
    dest[i] = 0;
}

// Replace z_open with kernel filesystem
int z_open(const char *filename, int flags) {
    // Write to a fixed VGA location that won't be overwritten
    // Use row 24 (near bottom) to avoid screen clearing
    volatile char* vga = (volatile char*)0xB8000;
    int offset = 24 * 80 * 2;  // Row 24
    vga[offset + 0] = 'Z';
    vga[offset + 1] = 0x0F;  // White on black
    vga[offset + 2] = 'O';
    vga[offset + 3] = 0x0F;
    vga[offset + 4] = 'P';
    vga[offset + 5] = 0x0F;
    vga[offset + 6] = 'E';
    vga[offset + 7] = 0x0F;
    vga[offset + 8] = 'N';
    vga[offset + 9] = 0x0F;
    
    if (next_fd >= 16) {
        vga[offset + 10] = 'F';
        vga[offset + 11] = 0x0C;  // Red
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
        vga[12] = 'F';
        vga[14] = 'A';
        vga[16] = 'I';
        vga[18] = 'L';
        return -1;
    }
    
    vga[12] = 'O';
    vga[14] = 'K';
    
    kernel_file_t* f = &kernel_files[next_fd];
    f->filename = (char*)filename;
    f->buffer = loaded_buffer;
    f->size = loaded_size;
    f->pos = 0;
    
    return next_fd++;
}

// Replace z_read with kernel buffer
ssize_t z_read(int fd, void *buf, size_t count) {
    if (fd < 3 || fd >= 16) return -1;
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
    if (fd < 3 || fd >= 16) return -1;
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
    if (fd < 3 || fd >= 16) return -1;
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
int z_exit(int status) {
    // This will be handled by exit handler
    extern void elf_exit_program();
    elf_exit_program();
    return 0;
}

// Global variables for stack restoration
uint32_t saved_kernel_esp = 0;
uint32_t saved_kernel_ebp = 0;
uint32_t saved_kernel_esp_for_exit = 0;
uint32_t saved_kernel_ebp_for_exit = 0;
void* elf_exit_label_addr = 0;
int program_exit_requested = 0;

// Main entry point wrapper
int elf_load_and_run(const char* filename) {
    // Build stack: argc=2, argv[0]="loader", argv[1]=filename, argv[2]=NULL, envp=NULL, auxv=AT_NULL
    // Stack layout (from top to bottom):
    //   auxv[1] = 0
    //   auxv[0] = AT_NULL
    //   envp = NULL
    //   argv[2] = NULL
    //   argv[1] = pointer to filename string
    //   argv[0] = pointer to "loader" string
    //   argc = 2
    //   (strings at lower addresses)
    
    uint32_t user_stack_top = 0x800000 + 0x100000;  // USER_STACK_START + USER_STACK_SIZE
    uint32_t* stack = (uint32_t*)user_stack_top;
    
    // First, allocate space for strings at lower addresses
    stack -= 128;  // Space for strings
    char* argv0_str = (char*)stack;
    char* argv1_str = (char*)stack + 64;
    
    // Copy "loader" string
    int pos = 0;
    const char* loader_str = "loader";
    while (loader_str[pos]) {
        argv0_str[pos] = loader_str[pos];
        pos++;
    }
    argv0_str[pos] = 0;
    
    // Copy filename string
    pos = 0;
    while (filename[pos]) {
        argv1_str[pos] = filename[pos];
        pos++;
    }
    argv1_str[pos] = 0;
    
    // Now build the stack frame from top down
    // auxv[1] = 0
    stack = (uint32_t*)user_stack_top;
    stack -= 1;
    stack[0] = 0;  // auxv[1].a_val
    
    // auxv[0] = AT_NULL
    stack -= 1;
    stack[0] = AT_NULL;  // auxv[0].a_type
    
    // envp = NULL
    stack -= 1;
    stack[0] = 0;  // envp
    
    // argv[2] = NULL
    stack -= 1;
    stack[0] = 0;  // argv[2]
    
    // argv[1] = pointer to filename
    stack -= 1;
    stack[0] = (uint32_t)argv1_str;  // argv[1]
    
    // argv[0] = pointer to "loader"
    stack -= 1;
    stack[0] = (uint32_t)argv0_str;  // argv[0]
    
    // argc = 2
    stack -= 1;
    stack[0] = 2;  // argc
    
    // sp points to argc (top of frame)
    unsigned long* sp = (unsigned long*)stack;
    uint32_t user_sp_value = (uint32_t)sp;
    
    // Debug: check what we're passing
    extern void print(const char*);
    print("DEBUG: user ESP=");
    {
        char buf[16];
        int pos = 0;
        uint32_t val = user_sp_value;
        if (val == 0) buf[pos++] = '0';
        else {
            char tmp_buf[16];
            int tpos = 0;
            while (val > 0 && tpos < 8) {
                int nib = val & 0xF;
                tmp_buf[tpos++] = (nib < 10) ? ('0' + nib) : ('A' + nib - 10);
                val >>= 4;
            }
            while (tpos--) buf[pos++] = tmp_buf[tpos];
        }
        buf[pos] = 0;
        print(buf);
        print("\n");
    }
    print("DEBUG: argc=");
    char argc_str[16];
    int argc_val = (int)sp[0];
    int argc_pos = 0;
    if (argc_val == 0) argc_str[argc_pos++] = '0';
    else {
        int tmp = argc_val;
        while (tmp > 0 && argc_pos < 15) {
            argc_str[argc_pos++] = '0' + (tmp % 10);
            tmp /= 10;
        }
    }
    for (int j = argc_pos-1; j >= 0; j--) putchar(argc_str[j]);
    print("\n");
    print("DEBUG: argv[0]=");
    if (sp[1]) print((char*)sp[1]); else print("NULL");
    print("\n");
    print("DEBUG: argv[1]=");
    if (sp[2]) print((char*)sp[2]); else print("NULL");
    print("\n");
    
    // Save kernel stack
    void* exit_label = &&exit_cleanup;
    __asm__ volatile("mov %%esp, %0" : "=r"(saved_kernel_esp)); // YA SAVED KERNEL EFI SYSTEM PARTITION NE AMK AĞĞĞĞĞĞĞĞĞĞĞĞĞĞĞĞĞĞĞĞĞĞĞĞĞĞĞĞ
    __asm__ volatile("mov %%ebp, %0" : "=r"(saved_kernel_ebp));
    saved_kernel_esp_for_exit = saved_kernel_esp;
    saved_kernel_ebp_for_exit = saved_kernel_ebp;
    elf_exit_label_addr = exit_label;
    
    // Mask hardware interrupts
    __asm__ volatile("outb %%al, %%dx" : : "a"((uint8_t)0xFF), "d"((uint16_t)0x21));
    __asm__ volatile("outb %%al, %%dx" : : "a"((uint8_t)0xFF), "d"((uint16_t)0xA1));
    
    // Enable interrupts
    __asm__ volatile("sti");
    
    // Call z_entry
    extern void z_entry(unsigned long *sp, void (*fini)(void));
    extern void z_fini(void);
    z_entry(sp, z_fini);
    
    // Restore kernel stack
    __asm__ volatile("cli");
    __asm__ volatile("mov %0, %%esp" : : "r"(saved_kernel_esp) : "memory");
    __asm__ volatile("mov %0, %%ebp" : : "r"(saved_kernel_ebp) : "memory");
    
    exit_cleanup:
    saved_kernel_esp = 0;
    saved_kernel_ebp = 0;
    elf_exit_label_addr = 0;
    
    return 0;
}

// Exit handler
void elf_exit_program() {
    program_exit_requested = 1;
    if (saved_kernel_esp != 0) {
        __asm__ volatile("cli");
        __asm__ volatile("mov %0, %%esp" : : "r"(saved_kernel_esp) : "memory");
        __asm__ volatile("mov %0, %%ebp" : : "r"(saved_kernel_ebp) : "memory");
        if (elf_exit_label_addr) {
            void* label = elf_exit_label_addr;
            elf_exit_label_addr = 0;
            goto *label;
        }
    }
}

// Stub for execve (not implemented yet)
int elf_load_and_execve(const char* filename, char* const argv[], char* const envp[]) {
    // TODO: Implement execve
    return -1;
}

// Fault recovery handler
void elf_fault_recovery() {
    // Restore kernel stack immediately
    if (saved_kernel_esp_for_exit != 0 && saved_kernel_ebp_for_exit != 0) {
        __asm__ volatile("cli");
        __asm__ volatile("mov %0, %%esp" : : "r"(saved_kernel_esp_for_exit) : "memory");
        __asm__ volatile("mov %0, %%ebp" : : "r"(saved_kernel_ebp_for_exit) : "memory");
        
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

