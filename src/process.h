#ifndef PROCESS_H
#define PROCESS_H

// Kendi typedef'lerimiz
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;

// Process states
#define PROCESS_READY 0
#define PROCESS_RUNNING 1
#define PROCESS_BLOCKED 2
#define PROCESS_TERMINATED 3

// Process structure
struct process {
    uint32_t pid;
    uint32_t state;
    uint32_t esp;
    uint32_t ebp;
    uint32_t eip;
    uint32_t stack;
    uint32_t stack_size;
    char name[32];
    struct process* next;
};

// Utility fonksiyonları
void strcpy(char* dest, char* src);
int strcmp(char* s1, char* s2);
int strncmp(char* s1, char* s2, int n);
int strlen(const char* str);

// Process management fonksiyonları
void process_init();
uint32_t process_create(char* name, void* entry_point);
void process_schedule();
void process_yield();
void process_exit(uint32_t pid);

// Current process
extern struct process* current_process;
extern uint32_t next_pid;

#endif 