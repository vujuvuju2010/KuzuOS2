#include "process.h"
#include "memory.h"

#define MAX_PROCESSES 10
#define PROCESS_STACK_SIZE 4096

// Basit strcpy fonksiyonu
void strcpy(char* dest, char* src) {
    while (*src) {
        *dest = *src;
        dest++;
        src++;
    }
    *dest = 0;
}

// Basit strcmp fonksiyonu
int strcmp(char* s1, char* s2) {
    while (*s1 && *s2) {
        if ((unsigned char)*s1 != (unsigned char)*s2) return (unsigned char)*s1 - (unsigned char)*s2;
        s1++;
        s2++;
    }
    return (unsigned char)*s1 - (unsigned char)*s2;
}

// Basit strncmp fonksiyonu
int strncmp(char* s1, char* s2, int n) {
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

// Process list
struct process* process_list = 0;
struct process* current_process = 0;
uint32_t next_pid = 1;

void process_init() {
    // İlk process'i oluştur (kernel process)
    current_process = (struct process*)kmalloc(sizeof(struct process));
    current_process->pid = 0;
    current_process->state = PROCESS_RUNNING;
    current_process->stack = 0;
    current_process->stack_size = 0;
    strcpy(current_process->name, "kernel");
    current_process->next = 0;
    process_list = current_process;
}

uint32_t process_create(char* name, void* entry_point) {
    if (next_pid >= MAX_PROCESSES) {
        return 0; // Process limit reached
    }
    
    // Yeni process oluştur
    struct process* new_process = (struct process*)kmalloc(sizeof(struct process));
    new_process->pid = next_pid++;
    new_process->state = PROCESS_READY;
    new_process->stack = (uint32_t)kmalloc(PROCESS_STACK_SIZE);
    new_process->stack_size = PROCESS_STACK_SIZE;
    new_process->eip = (uint32_t)entry_point;
    new_process->esp = new_process->stack + PROCESS_STACK_SIZE - 16;
    new_process->ebp = new_process->esp;
    strcpy(new_process->name, name);
    
    // Process list'e ekle
    new_process->next = process_list;
    process_list = new_process;
    
    return new_process->pid;
}

void process_schedule() {
    if (!current_process || !process_list) {
        return;
    }
    
    // Round-robin scheduling
    struct process* next = current_process->next;
    if (!next) {
        next = process_list;
    }
    
    // Ready state'deki process'i bul
    while (next != current_process) {
        if (next->state == PROCESS_READY) {
            // Context switch
            current_process->state = PROCESS_READY;
            next->state = PROCESS_RUNNING;
            current_process = next;
            return;
        }
        next = next->next;
        if (!next) {
            next = process_list;
        }
    }
}

void process_yield() {
    process_schedule();
}

void process_exit(uint32_t pid) {
    struct process* p = process_list;
    while (p) {
        if (p->pid == pid) {
            p->state = PROCESS_TERMINATED;
            // Stack'i free et
            if (p->stack) {
                kfree((void*)p->stack);
            }
            break;
        }
        p = p->next;
    }
} 