#include "smp.h"
#include "memory.h"
#include "vga.h"
#include "process.h"

// Global CPU data
struct cpu_data g_cpu_data[MAX_CPU_CORES];
uint32_t g_cpu_count = 1;  // Default to 1 CPU (boot CPU)
uint32_t g_boot_cpu_id = 0;

// Track which CPUs have high priority processes assigned
static uint8_t cpu_has_high_priority[MAX_CPU_CORES];

// Initialize SMP subsystem
void smp_init(void) {
    // Initialize all CPU data structures
    for (uint32_t i = 0; i < MAX_CPU_CORES; i++) {
        g_cpu_data[i].cpu_id = i;
        g_cpu_data[i].is_present = (i == 0) ? 1 : 0;  // Only boot CPU present initially
        g_cpu_data[i].is_boot_cpu = (i == 0) ? 1 : 0;
        g_cpu_data[i].is_halted = (i == 0) ? 0 : 1;
        g_cpu_data[i].current_process_cpu_affinity = 0xFF;
        g_cpu_data[i].cpu_runqueue_head = 0;
        g_cpu_data[i].cpu_runqueue_tail = 0;
        g_cpu_data[i].runqueue_count = 0;
        g_cpu_data[i].current_process = 0;
        g_cpu_data[i].stack_base = 0;
        g_cpu_data[i].stack_size = 0;
        g_cpu_data[i].context_switches = 0;
        g_cpu_data[i].idle_ticks = 0;
        cpu_has_high_priority[i] = 0;
    }
    
    // Set boot CPU as current
    g_cpu_data[0].current_process = current_process;
    
    print_color("[SMP] Initialized with ", VGA_COLOR_LIGHT_GREEN);
    char buf[16];
    int pos = 0;
    uint32_t n = g_cpu_count;
    if (n == 0) buf[pos++] = '0';
    else {
        while (n > 0) {
            buf[pos++] = '0' + (n % 10);
            n /= 10;
        }
    }
    buf[pos] = '\0';
    for (int j = pos - 1; j >= 0; j--) putchar(buf[j]);
    print_color(" CPU(s)\n", VGA_COLOR_LIGHT_GREEN);
}

// Initialize a specific CPU
void smp_cpu_init(uint32_t cpu_id) {
    if (cpu_id >= MAX_CPU_CORES) return;
    
    g_cpu_data[cpu_id].is_present = 1;
    g_cpu_data[cpu_id].is_halted = 0;
    
    // Allocate stack for this CPU
    g_cpu_data[cpu_id].stack_size = 16384;  // 16KB per CPU stack
    g_cpu_data[cpu_id].stack_base = (uint64_t)kmalloc(g_cpu_data[cpu_id].stack_size);
    
    if (cpu_id > 0) {
        print_color("[SMP] CPU ", VGA_COLOR_LIGHT_GREEN);
        // Print cpu_id
        int pos = 0;
        uint32_t n = cpu_id;
        char buf[16];
        if (n == 0) buf[pos++] = '0';
        else {
            while (n > 0) {
                buf[pos++] = '0' + (n % 10);
                n /= 10;
            }
        }
        buf[pos] = '\0';
        for (int j = pos - 1; j >= 0; j--) putchar(buf[j]);
        print_color(" initialized\n", VGA_COLOR_LIGHT_GREEN);
    }
}

// Halt a CPU
void smp_cpu_halt(uint32_t cpu_id) {
    if (cpu_id >= MAX_CPU_CORES) return;
    g_cpu_data[cpu_id].is_halted = 1;
}

// Start a CPU (bring out of halt)
void smp_cpu_start(uint32_t cpu_id) {
    if (cpu_id >= MAX_CPU_CORES) return;
    g_cpu_data[cpu_id].is_halted = 0;
}

// Assign a process to a specific CPU core
void process_assign_to_cpu(struct process* proc, uint32_t cpu_id) {
    if (!proc || cpu_id >= MAX_CPU_CORES) return;
    
    proc->cpu_affinity = (uint8_t)cpu_id;
    
    // If high priority, mark the CPU as having a high priority process
    if (proc->is_high_priority) {
        cpu_has_high_priority[cpu_id] = 1;
    }
}

// Add a process to a CPU's runqueue
void cpu_add_to_runqueue(uint32_t cpu_id, struct process* proc) {
    if (!proc || cpu_id >= MAX_CPU_CORES) return;
    
    struct cpu_data* cpu = &g_cpu_data[cpu_id];
    
    // Check if process is already in a runqueue
    if (proc->cpu_affinity != 0xFF && proc->cpu_affinity != cpu_id) {
        // Process is assigned to a different CPU
        return;
    }
    
    // Add to tail of runqueue
    proc->next = 0;
    proc->prev = cpu->cpu_runqueue_tail;
    
    if (cpu->cpu_runqueue_tail) {
        cpu->cpu_runqueue_tail->next = proc;
    } else {
        cpu->cpu_runqueue_head = proc;
    }
    
    cpu->cpu_runqueue_tail = proc;
    cpu->runqueue_count++;
}

// Remove a process from a CPU's runqueue
void cpu_remove_from_runqueue(uint32_t cpu_id, struct process* proc) {
    if (!proc || cpu_id >= MAX_CPU_CORES) return;
    
    struct cpu_data* cpu = &g_cpu_data[cpu_id];
    
    if (proc->prev) {
        proc->prev->next = proc->next;
    } else {
        cpu->cpu_runqueue_head = proc->next;
    }
    
    if (proc->next) {
        proc->next->prev = proc->prev;
    } else {
        cpu->cpu_runqueue_tail = proc->prev;
    }
    
    if (cpu->runqueue_count > 0) {
        cpu->runqueue_count--;
    }
}

// Get next process to run on a CPU
struct process* cpu_get_next_process(uint32_t cpu_id) {
    if (cpu_id >= MAX_CPU_CORES) return 0;
    
    struct cpu_data* cpu = &g_cpu_data[cpu_id];
    
    // First, check for high priority processes
    struct process* p = cpu->cpu_runqueue_head;
    while (p) {
        if (p->is_high_priority && p->state == PROCESS_READY) {
            return p;
        }
        p = p->next;
    }
    
    // Then check for any ready process
    p = cpu->cpu_runqueue_head;
    while (p) {
        if (p->state == PROCESS_READY || p->state == PROCESS_RUNNING) {
            return p;
        }
        p = p->next;
    }
    
    return 0;
}

// Get current CPU ID (simplified - returns 0 for now)
uint32_t smp_get_current_cpu_id(void) {
    // In a real SMP implementation, this would read from a CPU-specific register
    // For now, we're single-threaded so always return 0
    return 0;
}

// Get current CPU data
struct cpu_data* smp_get_current_cpu_data(void) {
    return &g_cpu_data[smp_get_current_cpu_id()];
}

// Get CPU data by ID
struct cpu_data* smp_get_cpu_data(uint32_t cpu_id) {
    if (cpu_id >= MAX_CPU_CORES) return 0;
    return &g_cpu_data[cpu_id];
}

// Initialize a high priority process
void high_priority_process_init(struct process* proc) {
    if (!proc) return;
    
    proc->is_high_priority = 1;
    
    // On single-core systems (g_cpu_count == 1), all processes run on CPU 0
    // High priority just means they get scheduled first on that core
    if (g_cpu_count <= 1) {
        proc->cpu_affinity = 0;  // Run on boot CPU
        process_assign_to_cpu(proc, 0);
        return;
    }
    
    // Multi-core: High priority processes get assigned to a dedicated core
    // Find a core without a high priority process
    for (uint32_t i = 1; i < MAX_CPU_CORES && i < g_cpu_count; i++) {  // Start from 1 to leave CPU 0 for shell
        if (!cpu_has_high_priority[i]) {
            proc->cpu_affinity = (uint8_t)i;
            cpu_has_high_priority[i] = 1;
            process_assign_to_cpu(proc, i);
            return;
        }
    }
    
    // If no free core, assign to CPU 1 (or CPU 0 if only 1 core exists)
    uint32_t fallback_core = (g_cpu_count > 1) ? 1 : 0;
    proc->cpu_affinity = fallback_core;
    cpu_has_high_priority[fallback_core] = 1;
    process_assign_to_cpu(proc, fallback_core);
}

// Schedule a high priority process on its dedicated core
void smp_schedule_high_priority(struct process* proc) {
    if (!proc || !proc->is_high_priority) return;
    
    // Add to the runqueue of the assigned CPU
    cpu_add_to_runqueue(proc->cpu_affinity, proc);
}
