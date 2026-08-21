#ifndef VMM_H
#define VMM_H
 
#include <stdint.h> //ts gon shi itself
#include <stddef.h>

// Virtual Memory Manager - manages x64 4-level page tables

// Page table entry flags
#define PAGE_PRESENT    (1ULL << 0)
#define PAGE_WRITABLE   (1ULL << 1)
#define PAGE_USER       (1ULL << 2)
#define PAGE_WRITETHROUGH (1ULL << 3)
#define PAGE_CACHE_DISABLE (1ULL << 4)
#define PAGE_ACCESSED   (1ULL << 5)
#define PAGE_DIRTY      (1ULL << 6)
#define PAGE_HUGE       (1ULL << 7)  // PS bit - For 2MB/1GB pages
#define PAGE_SIZE_BIT   (1ULL << 7)  // Same as PAGE_HUGE (deprecated name)
#define PAGE_GLOBAL     (1ULL << 8)
#define PAGE_NX         (1ULL << 63) // No execute

// Virtual address breakdown
#define PML4_INDEX(addr) (((addr) >> 39) & 0x1FF)
#define PDPT_INDEX(addr) (((addr) >> 30) & 0x1FF)
#define PD_INDEX(addr)   (((addr) >> 21) & 0x1FF)
#define PT_INDEX(addr)   (((addr) >> 12) & 0x1FF)

// Page table entry (8 bytes)
typedef uint64_t pte_t;

// Page table structure (512 entries, 4KB)
typedef struct {
    pte_t entries[512];
} __attribute__((aligned(4096))) page_table_t;

// Initialize VMM - uses existing boot.asm page tables
void vmm_init(void);

// Map framebuffer memory (call after vmm_init and pmm_init)
void vmm_map_framebuffer(void);

// Map a virtual address to a physical address with specified flags
// Creates intermediate page tables as needed
int vmm_map_page(uint64_t virt_addr, uint64_t phys_addr, uint64_t flags);

// Unmap a virtual address (optional - can skip for minimum implementation)
void vmm_unmap_page(uint64_t virt_addr);

// Get the physical address mapped at a virtual address
// Returns 0 if not mapped
uint64_t vmm_get_physical(uint64_t virt_addr);

// Get current PML4 physical address
uint64_t vmm_get_pml4(void);

// Switch to the new dynamically-built page tables
void vmm_switch_to_new_tables(void);

#endif
