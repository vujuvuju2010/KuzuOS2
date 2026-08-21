// Virtual Memory Manager - 4-level x64 paging
#include "vmm.h"
#include "pmm.h"

// Use boot.asm page tables initially
extern page_table_t pml4;
extern page_table_t pdpt;
extern page_table_t pd_low;
extern page_table_t pd_high;

// Current kernel PML4 (top-level page table)
static page_table_t* kernel_pml4 = NULL;

// Helper: zero out a page table
static void zero_page_table(page_table_t* pt) {
    for (int i = 0; i < 512; i++) {
        pt->entries[i] = 0;
    }
}


static page_table_t* get_or_create_table(pte_t* parent_entry, uint64_t flags) {
    if (*parent_entry & PAGE_PRESENT) {
        // Entry already exists, extract physical address
        uint64_t phys_addr = *parent_entry & 0x000FFFFFFFFFF000ULL;
        return (page_table_t*)phys_addr;
    } else {
        // Need to allocate a new page table
        uint64_t phys_addr = pmm_alloc_frame();
        if (phys_addr == 0) {
            return NULL; // Out of memory
        }
        
        // Zero it out
        page_table_t* new_table = (page_table_t*)phys_addr;
        zero_page_table(new_table);
        
        // Link it into parent with appropriate flags
        *parent_entry = phys_addr | (flags & 0x7) | PAGE_PRESENT;
        
        return new_table;
    }
}

void vmm_init(void) {
    // Use boot.asm page tables (they have 0-2GB mapped)
    kernel_pml4 = &pml4;
    
    // Boot.asm already mapped 0-2GB which is enough for now
    // Framebuffer mapping will be added later if needed
}

void vmm_map_framebuffer(void) {
    // Map framebuffer region
    extern uint64_t framebuffer;
    uint64_t fb_addr = framebuffer;
    
    if (fb_addr != 0 && fb_addr >= 0x80000000ULL) {
        // Framebuffer is above 2GB, map it
        // Map 64MB around framebuffer
        uint64_t fb_base = fb_addr & ~0x3FFFFFFULL; // Align to 64MB
        uint64_t fb_end = fb_base + (64ULL * 1024 * 1024);
        
        for (uint64_t addr = fb_base; addr < fb_end; addr += PAGE_SIZE) {
            vmm_map_page(addr, addr, PAGE_PRESENT | PAGE_WRITABLE);
        }
    }
}

int vmm_map_page(uint64_t virt_addr, uint64_t phys_addr, uint64_t flags) {
    if (kernel_pml4 == NULL) {
        return -1;
    }

    // Addresses in these ranges are already mapped by boot.asm's 2MB huge
    // pages (pd_low / pd_high). get_or_create_table cannot walk into a huge
    // page, so don't even try — treat these as already-mapped.
    if (virt_addr < 0x40000000ULL ||
        (virt_addr >= 0xC0000000ULL && virt_addr < 0x100000000ULL)) {
        return 0; // already mapped by boot.asm, nothing to do
    }

    uint64_t pml4_idx = PML4_INDEX(virt_addr);
    uint64_t pdpt_idx = PDPT_INDEX(virt_addr);
    uint64_t pd_idx = PD_INDEX(virt_addr);
    uint64_t pt_idx = PT_INDEX(virt_addr);

    page_table_t* pdpt = get_or_create_table(&kernel_pml4->entries[pml4_idx], PAGE_WRITABLE | PAGE_USER);
    if (pdpt == NULL) return -1;

    page_table_t* pd = get_or_create_table(&pdpt->entries[pdpt_idx], PAGE_WRITABLE | PAGE_USER);
    if (pd == NULL) return -1;

    page_table_t* pt = get_or_create_table(&pd->entries[pd_idx], PAGE_WRITABLE | PAGE_USER);
    if (pt == NULL) return -1;

    pt->entries[pt_idx] = (phys_addr & 0x000FFFFFFFFFF000ULL) | (flags & 0xFFF) | PAGE_PRESENT;
    asm volatile("invlpg (%0)" : : "r"(virt_addr) : "memory");
    return 0;
}

void vmm_unmap_page(uint64_t virt_addr) {
    if (kernel_pml4 == NULL) {
        return;
    }
    
    uint64_t pml4_idx = PML4_INDEX(virt_addr);
    uint64_t pdpt_idx = PDPT_INDEX(virt_addr);
    uint64_t pd_idx = PD_INDEX(virt_addr);
    uint64_t pt_idx = PT_INDEX(virt_addr);
    
    // Walk to the PT level
    if (!(kernel_pml4->entries[pml4_idx] & PAGE_PRESENT)) return;
    page_table_t* pdpt = (page_table_t*)(kernel_pml4->entries[pml4_idx] & 0x000FFFFFFFFFF000ULL);
    
    if (!(pdpt->entries[pdpt_idx] & PAGE_PRESENT)) return;
    page_table_t* pd = (page_table_t*)(pdpt->entries[pdpt_idx] & 0x000FFFFFFFFFF000ULL);
    
    if (!(pd->entries[pd_idx] & PAGE_PRESENT)) return;
    page_table_t* pt = (page_table_t*)(pd->entries[pd_idx] & 0x000FFFFFFFFFF000ULL);
    
    // Clear the entry
    pt->entries[pt_idx] = 0;
    
    // Invalidate TLB
    asm volatile("invlpg (%0)" : : "r"(virt_addr) : "memory");
}

uint64_t vmm_get_physical(uint64_t virt_addr) {
    if (kernel_pml4 == NULL) {
        return 0;
    }
    
    uint64_t pml4_idx = PML4_INDEX(virt_addr);
    uint64_t pdpt_idx = PDPT_INDEX(virt_addr);
    uint64_t pd_idx = PD_INDEX(virt_addr);
    uint64_t pt_idx = PT_INDEX(virt_addr);
    
    // Walk the page tables
    if (!(kernel_pml4->entries[pml4_idx] & PAGE_PRESENT)) return 0;
    page_table_t* pdpt = (page_table_t*)(kernel_pml4->entries[pml4_idx] & 0x000FFFFFFFFFF000ULL);
    
    if (!(pdpt->entries[pdpt_idx] & PAGE_PRESENT)) return 0;
    page_table_t* pd = (page_table_t*)(pdpt->entries[pdpt_idx] & 0x000FFFFFFFFFF000ULL);
    
    if (!(pd->entries[pd_idx] & PAGE_PRESENT)) return 0;
    page_table_t* pt = (page_table_t*)(pd->entries[pd_idx] & 0x000FFFFFFFFFF000ULL);
    
    if (!(pt->entries[pt_idx] & PAGE_PRESENT)) return 0;
    
    uint64_t phys_base = pt->entries[pt_idx] & 0x000FFFFFFFFFF000ULL;
    uint64_t offset = virt_addr & 0xFFF;
    
    return phys_base + offset;
}

uint64_t vmm_get_pml4(void) {
    return (uint64_t)kernel_pml4;
}

void vmm_switch_to_new_tables(void) {
    if (kernel_pml4 == NULL) {
        return;
    }
    
    // Load new PML4 into CR3
    uint64_t pml4_phys = (uint64_t)kernel_pml4;
    asm volatile(
        "mov %0, %%cr3"
        :
        : "r"(pml4_phys)
        : "memory"
    );
}
