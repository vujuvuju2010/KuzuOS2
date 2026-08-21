#ifndef PMM_H
#define PMM_H

#include <stdint.h>
#include <stddef.h>

// Physical Memory Manager - tracks free/used 4KB page frames

#define PAGE_SIZE 4096
#define FRAME_SIZE PAGE_SIZE

// Initialize PMM with Multiboot2 memory map
void pmm_init(uint32_t mb_addr);

// Allocate a single 4KB physical frame
// Returns physical address, or 0 if out of memory
uint64_t pmm_alloc_frame(void);

// Free a single 4KB physical frame
void pmm_free_frame(uint64_t phys_addr);

// Get total/used/free memory stats
uint64_t pmm_get_total_memory(void);
uint64_t pmm_get_used_memory(void);
uint64_t pmm_get_free_memory(void);

#endif
