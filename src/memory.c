#include "memory.h"

#define HEAP_START 0x1000000  // 16MB'da başla
#define HEAP_SIZE 0x10000000  // 256MB heap (increased from 64MB to handle large allocations)

static struct memory_block* heap_start = (struct memory_block*)HEAP_START;
static uint8_t heap_initialized = 0;

void memory_init() {
    if (heap_initialized) return;
    
    // İlk block'u ayarla
    heap_start->size = HEAP_SIZE - sizeof(struct memory_block);
    heap_start->used = 0;
    heap_start->next = 0;
    
    heap_initialized = 1;
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