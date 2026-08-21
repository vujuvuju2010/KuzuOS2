/* libc/src/stdlib/malloc.c */
#include <stdlib.h>
#include <stdint.h>

/* Simple malloc/free using kernel kmalloc/kfree */
extern void* kmalloc(uint32_t size);
extern void kfree(void* ptr);

void* malloc(size_t size) {
    if (size == 0) return NULL;
    return kmalloc((uint32_t)size);
}

void* calloc(size_t nmemb, size_t size) {
    size_t total = nmemb * size;
    void* ptr = malloc(total);
    if (ptr) {
        unsigned char* p = (unsigned char*)ptr;
        for (size_t i = 0; i < total; i++)
            p[i] = 0;
    }
    return ptr;
}

void* realloc(void* ptr, size_t size) {
    if (size == 0) {
        free(ptr);
        return NULL;
    }
    
    void* new_ptr = malloc(size);
    if (!new_ptr) return NULL;
    
    if (ptr) {
        /* Copy memory - without tracking old size, we copy up to new size */
        unsigned char* src = (unsigned char*)ptr;
        unsigned char* dst = (unsigned char*)new_ptr;
        /* This is a simplified realloc; real implementation would track block sizes */
        for (size_t i = 0; i < size; i++)
            dst[i] = src[i];
        free(ptr);
    }
    
    return new_ptr;
}

void free(void* ptr) {
    if (ptr)
        kfree(ptr);
}
