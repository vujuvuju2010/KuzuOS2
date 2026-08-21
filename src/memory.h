#ifndef MEMORY_H
#define MEMORY_H

// Memory management için temel yapılar
typedef unsigned int uint32_t;
typedef unsigned char uint8_t;

// Memory block yapısı
struct memory_block {
    uint32_t size;
    uint8_t used;
    struct memory_block* next;
};

// Memory manager fonksiyonları
void memory_init();
void memory_init_with_size(uint32_t available_memory);
void* kmalloc(uint32_t size);
void kfree(void* ptr);
void* memcpy(void* dst, const void* src, uint32_t n);

#endif 