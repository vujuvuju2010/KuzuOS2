// Physical Memory Manager - bitmap-based frame allocator
#include "pmm.h"

// Multiboot2 tag types
#define MB2_TAG_END 0
#define MB2_TAG_BASIC_MEMINFO 4
#define MB2_TAG_MMAP 6

// Memory map entry types
#define MB2_MMAP_AVAILABLE 1
#define MB2_MMAP_RESERVED 2
#define MB2_MMAP_ACPI_RECLAIMABLE 3
#define MB2_MMAP_NVS 4
#define MB2_MMAP_BADRAM 5

// Kernel location (from linker)
extern char _kernel_start;
extern char _kernel_end;

// Bitmap to track page frames (1 bit per 4KB frame)
static uint32_t* frame_bitmap = NULL;
static uint64_t total_frames = 0;
static uint64_t used_frames = 0;
static uint64_t bitmap_size_bytes = 0;

// Bitmap will be placed right after kernel end
static uint64_t bitmap_phys_start = 0;

// Set a bit in the bitmap (mark frame as used)
static inline void bitmap_set(uint64_t frame) {
    uint64_t idx = frame / 32;
    uint64_t bit = frame % 32;
    frame_bitmap[idx] |= (1 << bit);
}

// Clear a bit in the bitmap (mark frame as free)
static inline void bitmap_clear(uint64_t frame) {
    uint64_t idx = frame / 32;
    uint64_t bit = frame % 32;
    frame_bitmap[idx] &= ~(1 << bit);
}

// Test a bit in the bitmap (check if frame is used)
static inline int bitmap_test(uint64_t frame) {
    uint64_t idx = frame / 32;
    uint64_t bit = frame % 32;
    return (frame_bitmap[idx] & (1 << bit)) != 0;
}

void pmm_init(uint32_t mb_addr) {
    // Debug: print immediately to see if we enter this function
    extern void print_color(const char*, int);
    print_color("[PMM] Entered pmm_init\n", 0x0A);
    
    if (mb_addr == 0) {
        // Fallback: assume 128MB RAM if no multiboot info
        print_color("[PMM] No multiboot, using 128MB\n", 0x0E);
        total_frames = (128 * 1024 * 1024) / PAGE_SIZE;
    } else {
        print_color("[PMM] Parsing multiboot tags\n", 0x0A);
        // Parse Multiboot2 tags to find highest usable address
        uint32_t* tag_ptr = (uint32_t*)(uint64_t)(mb_addr + 8);
        uint64_t highest_addr = 0;
        
        while (1) {
            uint32_t tag_type = tag_ptr[0];
            uint32_t tag_size = tag_ptr[1];
            
            if (tag_type == MB2_TAG_END) {
                break;
            }
            
            // Memory map tag (type 6) - most detailed
            if (tag_type == MB2_TAG_MMAP) {
                uint32_t entry_size = tag_ptr[2];
                uint32_t entry_version = tag_ptr[3];
                
                uint8_t* entry_base = (uint8_t*)(tag_ptr + 4);
                uint32_t entries_data_size = tag_size - 16;
                
                for (uint32_t offset = 0; offset < entries_data_size; offset += entry_size) {
                    uint64_t* base_addr = (uint64_t*)(entry_base + offset);
                    uint64_t* length = (uint64_t*)(entry_base + offset + 8);
                    uint32_t* type = (uint32_t*)(entry_base + offset + 16);
                    
                    if (*type == MB2_MMAP_AVAILABLE) {
                        uint64_t end = *base_addr + *length;
                        if (end > highest_addr) {
                            highest_addr = end;
                        }
                    }
                }
            }
            
            // Move to next tag (8-byte aligned)
            uint32_t next_offset = (tag_size + 7) & ~7;
            tag_ptr = (uint32_t*)((uint8_t*)tag_ptr + next_offset);
        }
        
        if (highest_addr > 0) {
            total_frames = highest_addr / PAGE_SIZE;
        } else {
            // Fallback
            total_frames = (128 * 1024 * 1024) / PAGE_SIZE;
        }
    }
    
    // Calculate bitmap size (1 bit per frame)
    print_color("[PMM] Calculating bitmap size\n", 0x0A);
    bitmap_size_bytes = (total_frames + 7) / 8;
    bitmap_size_bytes = (bitmap_size_bytes + 7) & ~7; // 8-byte align
    
    // Place bitmap right after kernel end (align to page boundary)
    print_color("[PMM] Placing bitmap after kernel\n", 0x0A);
    uint64_t kernel_end = (uint64_t)&_kernel_end;
    bitmap_phys_start = (kernel_end + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
    frame_bitmap = (uint32_t*)bitmap_phys_start;
    
    // Mark all frames as used initially
    print_color("[PMM] Initializing bitmap (marking all as used)\n", 0x0A);
    for (uint64_t i = 0; i < bitmap_size_bytes / 4; i++) {
        frame_bitmap[i] = 0xFFFFFFFF;
    }
    print_color("[PMM] Bitmap initialized\n", 0x0A);
    used_frames = total_frames;
    
    // Now mark available regions as free by parsing memory map again
    if (mb_addr != 0) {
        uint32_t* tag_ptr = (uint32_t*)(uint64_t)(mb_addr + 8);
        
        while (1) {
            uint32_t tag_type = tag_ptr[0];
            uint32_t tag_size = tag_ptr[1];
            
            if (tag_type == MB2_TAG_END) {
                break;
            }
            
            if (tag_type == MB2_TAG_MMAP) {
                uint32_t entry_size = tag_ptr[2];
                uint8_t* entry_base = (uint8_t*)(tag_ptr + 4);
                uint32_t entries_data_size = tag_size - 16;
                
                for (uint32_t offset = 0; offset < entries_data_size; offset += entry_size) {
                    uint64_t* base_addr = (uint64_t*)(entry_base + offset);
                    uint64_t* length = (uint64_t*)(entry_base + offset + 8);
                    uint32_t* type = (uint32_t*)(entry_base + offset + 16);
                    
                    if (*type == MB2_MMAP_AVAILABLE) {
                        // Mark frames in this region as free
                        uint64_t start_frame = *base_addr / PAGE_SIZE;
                        uint64_t frame_count = *length / PAGE_SIZE;
                        
                        for (uint64_t f = start_frame; f < start_frame + frame_count && f < total_frames; f++) {
                            if (bitmap_test(f)) {
                                bitmap_clear(f);
                                used_frames--;
                            }
                        }
                    }
                }
            }
            
            uint32_t next_offset = (tag_size + 7) & ~7;
            tag_ptr = (uint32_t*)((uint8_t*)tag_ptr + next_offset);
        }
    }
    
    // Mark kernel's physical memory as used
    uint64_t kernel_start = (uint64_t)&_kernel_start;
    uint64_t kernel_start_frame = kernel_start / PAGE_SIZE;
    uint64_t kernel_end_frame = (kernel_end + PAGE_SIZE - 1) / PAGE_SIZE;
    
    for (uint64_t f = kernel_start_frame; f <= kernel_end_frame; f++) {
        if (!bitmap_test(f)) {
            bitmap_set(f);
            used_frames++;
        }
    }
    
    // Mark bitmap's own memory as used
    uint64_t bitmap_end = bitmap_phys_start + bitmap_size_bytes;
    uint64_t bitmap_start_frame = bitmap_phys_start / PAGE_SIZE;
    uint64_t bitmap_end_frame = (bitmap_end + PAGE_SIZE - 1) / PAGE_SIZE;
    
    for (uint64_t f = bitmap_start_frame; f <= bitmap_end_frame; f++) {
        if (!bitmap_test(f)) {
            bitmap_set(f);
            used_frames++;
        }
    }
    
    // Mark low memory (first 1MB) as used - BIOS/bootloader stuff
    for (uint64_t f = 0; f < (1024 * 1024) / PAGE_SIZE; f++) {
        if (!bitmap_test(f)) {
            bitmap_set(f);
            used_frames++;
        }
    }
}

uint64_t pmm_alloc_frame(void) {
    // Find first free frame
    for (uint64_t i = 0; i < total_frames; i++) {
        if (!bitmap_test(i)) {
            bitmap_set(i);
            used_frames++;
            return i * PAGE_SIZE;
        }
    }
    
    // Out of memory
    return 0;
}

void pmm_free_frame(uint64_t phys_addr) {
    uint64_t frame = phys_addr / PAGE_SIZE;
    
    if (frame >= total_frames) {
        return; // Invalid frame
    }
    
    if (bitmap_test(frame)) {
        bitmap_clear(frame);
        used_frames--;
    }
}

uint64_t pmm_get_total_memory(void) {
    return total_frames * PAGE_SIZE;
}

uint64_t pmm_get_used_memory(void) {
    return used_frames * PAGE_SIZE;
}

uint64_t pmm_get_free_memory(void) {
    return (total_frames - used_frames) * PAGE_SIZE;
}
