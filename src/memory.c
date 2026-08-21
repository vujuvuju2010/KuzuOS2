// not so tuff memory :(
#include "memory.h"

#define HEAP_START 0x10000000  // 256MB start (safe offset)
static uint32_t heap_size = 0x30000000;  // Default 768MB, will be updated by kernel

static struct memory_block* heap_start = (struct memory_block*)HEAP_START;
static uint8_t heap_initialized = 0;

void memory_init_with_size(uint32_t available_memory) {
    if (heap_initialized) return;
    
    // Calculate heap size: use 90% of available memory, minus kernel space
    // Reserve 256MB for kernel/IO, use rest for heap
    if (available_memory > 0x10000000) {
        heap_size = available_memory - 0x10000000;
        // Cap at 32GB - HEAP_START
        if (HEAP_START + heap_size > 0x800000000ULL) {
            heap_size = 0x800000000ULL - HEAP_START;
        }
    }
    
    // İlk block'u ayarla
    heap_start->size = heap_size - sizeof(struct memory_block);
    heap_start->used = 0;
    heap_start->next = 0;
    
    heap_initialized = 1;
}

void memory_init() {
    memory_init_with_size(0x100000000ULL);  // Default to 4GB if not specified
}

void* kmalloc(uint32_t size) {
    if (!heap_initialized) memory_init();
    
    struct memory_block* current = heap_start;
    
    while (current) {
        if (!current->used && current->size >= size) {
            // Bu block'u kullan
            current->used = 1;
            
            // Eğer block çok büyükse böl
            if (current->size > size + sizeof(struct memory_block) + 4) {
                struct memory_block* new_block = (struct memory_block*)((uint8_t*)current + sizeof(struct memory_block) + size);
                new_block->size = current->size - size - sizeof(struct memory_block);
                new_block->used = 0;
                new_block->next = current->next;
                
                current->size = size;
                current->next = new_block;
            }
            
            return (void*)((uint8_t*)current + sizeof(struct memory_block));
        }
        current = current->next;
    }
    
    // Defragment and try again
    struct memory_block* block = heap_start;
    while (block && block->next) {
        if (!block->used && !block->next->used) {
            // Merge adjacent free blocks
            block->size += sizeof(struct memory_block) + block->next->size;
            block->next = block->next->next;
        } else {
            block = block->next;
        }
    }
    
    // Try allocation again after defrag
    current = heap_start;
    while (current) {
        if (!current->used && current->size >= size) {
            current->used = 1;
            
            if (current->size > size + sizeof(struct memory_block) + 4) {
                struct memory_block* new_block = (struct memory_block*)((uint8_t*)current + sizeof(struct memory_block) + size);
                new_block->size = current->size - size - sizeof(struct memory_block);
                new_block->used = 0;
                new_block->next = current->next;
                
                current->size = size;
                current->next = new_block;
            }
            
            return (void*)((uint8_t*)current + sizeof(struct memory_block));
        }
        current = current->next;
    }
    
    return 0; // Memory yok
}

void kfree(void* ptr) {
    if (!ptr) return;
    
    struct memory_block* block = (struct memory_block*)((uint8_t*)ptr - sizeof(struct memory_block));
    block->used = 0;
    
    // Coalesce with next block if it's free
    if (block->next && !block->next->used) {
        block->size += sizeof(struct memory_block) + block->next->size;
        block->next = block->next->next;
    }
    
    // Coalesce with previous block if it's free
    struct memory_block* prev = heap_start;
    while (prev && prev->next != block) {
        prev = prev->next;
    }
    if (prev && !prev->used && prev != block) {
        prev->size += sizeof(struct memory_block) + block->size;
        prev->next = block->next;
    }
}

void heap_stats(void) {
    if (!heap_initialized) memory_init();
    
    struct memory_block* current = heap_start;
    uint32_t total_free = 0;
    uint32_t total_used = 0;
    uint32_t block_count = 0;
    uint32_t largest_free = 0;
    
    while (current) {
        block_count++;
        if (current->used) {
            total_used += current->size;
        } else {
            total_free += current->size;
            if (current->size > largest_free) {
                largest_free = current->size;
            }
        }
        current = current->next;
    }
    
    extern void z_printf(const char* fmt, ...);
    z_printf("[HEAP] Blocks: %d | Used: %x | Free: %x | Largest: %x\n", 
             block_count, total_used, total_free, largest_free);
}