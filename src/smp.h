#ifndef SMP_H
#define SMP_H

#include "process.h"

// Maximum number of CPU cores supported
#define MAX_CPU_CORES 16

// Per-CPU data structure
struct cpu_data {
    uint32_t cpu_id;              // CPU core ID
    uint8_t is_present;           // 1 if CPU is present/enabled
    uint8_t is_boot_cpu;          // 1 if this is the boot CPU (BSP)
    uint8_t is_halted;            // 1 if CPU is halted
    uint8_t current_process_cpu_affinity;  // Current process affinity
    
    // Per-CPU runqueue for processes assigned to this core
    struct process* cpu_runqueue_head;
    struct process* cpu_runqueue_tail;
    uint32_t runqueue_count;
    
    // Current running process on this CPU
    struct process* current_process;
    
    // Stack for this CPU
    uint64_t stack_base;
    uint64_t stack_size;
    
    // Statistics
    uint64_t context_switches;
    uint64_t idle_ticks;
};

// Global CPU data array
extern struct cpu_data g_cpu_data[MAX_CPU_CORES];
extern uint32_t g_cpu_count;
extern uint32_t g_boot_cpu_id;

// SMP initialization
void smp_init(void);
void smp_cpu_init(uint32_t cpu_id);
void smp_cpu_halt(uint32_t cpu_id);
void smp_cpu_start(uint32_t cpu_id);

// CPU affinity management
void process_assign_to_cpu(struct process* proc, uint32_t cpu_id);
struct process* cpu_get_next_process(uint32_t cpu_id);
void cpu_add_to_runqueue(uint32_t cpu_id, struct process* proc);
void cpu_remove_from_runqueue(uint32_t cpu_id, struct process* proc);

// Current CPU identification
uint32_t smp_get_current_cpu_id(void);
struct cpu_data* smp_get_current_cpu_data(void);
struct cpu_data* smp_get_cpu_data(uint32_t cpu_id);

// High priority process management
void high_priority_process_init(struct process* proc);
void smp_schedule_high_priority(struct process* proc);

#endif // SMP_H
