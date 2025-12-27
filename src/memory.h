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
void* kmalloc(uint32_t size);
void kfree(void* ptr);

#endif 