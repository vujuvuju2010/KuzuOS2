#ifndef ELF_H
#define ELF_H

typedef unsigned long long uint64_t;
typedef unsigned int uint32_t;
typedef unsigned short uint16_t;
typedef unsigned char uint8_t;

// ELF yükleyici fonksiyonu
typedef void (*entry_point_t)();

int elf_load_and_run(const char* filename);
int elf_load_and_execve(const char* filename, char* const argv[], char* const envp[]);

// Function to restore kernel stack and exit user program
void elf_exit_program();
void elf_exit_handler();

// Global variables (defined in loader_kernel.c)
extern uint64_t saved_kernel_esp;
extern uint64_t saved_kernel_ebp;
extern uint64_t saved_kernel_esp_for_exit;
extern uint64_t saved_kernel_ebp_for_exit;
extern void* elf_exit_label_addr;
extern int program_exit_requested;

// Get command-line args for current process (for syscall 250)
const char* get_current_cmd_args(void);

#endif 
