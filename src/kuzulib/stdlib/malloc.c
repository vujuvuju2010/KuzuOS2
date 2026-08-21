/*
 *tuffest memmory allocator everrr
 * ts is so tuf twin its so tuf that it got its own fukjing file jahoy  


 */

// okayyyy rn i have a eip issue which puts eip 15 lol soo okay unless you have that issue DO NOT TOUCH THIS FILEEEE PLEASE DONTTTTTT AGHJHHHHHHHHHHHHHHHHH
#include <stddef.h>
#include <stdint.h>

/* Define uintptr_t if not defined */
#ifndef __UINTPTR_TYPE__
typedef unsigned int uintptr_t;
#endif

/* Define SIZE_MAX if not defined */
#ifndef SIZE_MAX
#define SIZE_MAX ((size_t)-1)
#endif

/* Configuration */
#define HEAP_SIZE (256 * 1024 * 1024)  /* 256MB initial heap - will grow via sbrk if needed */
#define MIN_BLOCK_SIZE 32              /* Minimum allocation size */
#define ALIGNMENT 8                    /* 8-byte alignment */
#define NUM_SIZE_CLASSES 24            /* Number of segregated free lists */


#define DMA_BASE 0x200000 // 2mb falan edio i think
#define DMA_SIZE 0x10000 // 64kb QH ye falan yeter die dusunuom yetmezse çak bikaç sıfır daha sonuna 

static unsigned int dma_ptr = DMA_BASE; // immma point ma pointer to yo poimnter


/* Alignment macro */
#define ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~(ALIGNMENT - 1))

/* Block header structure - explicitly padded to 16 bytes on 32-bit */
typedef struct block_header {
    size_t size;                     /* Size includes header, LSB = allocated bit */
    struct block_header* next;       /* Next in free list */
    struct block_header* prev;       /* Previous in free list */
    size_t _pad;                     /* Pad to 16 bytes so HEADER_SIZE matches sizeof */
} block_header_t;

/* Block footer (only for free blocks) */
typedef struct block_footer {
    size_t size;                     /* Must match header size */
} block_footer_t;

/* Macros for block manipulation */
#define HEADER_SIZE 16 // these are hardcoded for 32 bit since it would stop spitting me a weird int 06 eip 15 error which was same as the header idk mann ağğğğ
#define FOOTER_SIZE 4 // its either 8 or 4 and 8 didnt workk
#define OVERHEAD (HEADER_SIZE + FOOTER_SIZE)

#define ALLOCATED_BIT 0x1
#define PREV_ALLOCATED_BIT 0x2
#define SIZE_MASK (~0x3)

#define GET_SIZE(block) ((block)->size & SIZE_MASK)
#define IS_ALLOCATED(block) ((block)->size & ALLOCATED_BIT)
#define IS_PREV_ALLOCATED(block) ((block)->size & PREV_ALLOCATED_BIT)

#define SET_SIZE(block, sz) ((block)->size = ((block)->size & 0x3) | (sz))
#define SET_ALLOCATED(block) ((block)->size |= ALLOCATED_BIT)
#define SET_FREE(block) ((block)->size &= ~ALLOCATED_BIT)
#define SET_PREV_ALLOCATED(block) ((block)->size |= PREV_ALLOCATED_BIT)
#define SET_PREV_FREE(block) ((block)->size &= ~PREV_ALLOCATED_BIT)

/* Get footer from header (only valid for free blocks) */
static inline block_footer_t* get_footer(block_header_t* block) {
    return (block_footer_t*)((char*)block + GET_SIZE(block) - FOOTER_SIZE);
}

/* Get header from footer */
static inline block_header_t* get_header_from_footer(block_footer_t* footer) {
    return (block_header_t*)((char*)footer + FOOTER_SIZE - footer->size);
}

/* Get next physical block */
static inline block_header_t* get_next_block(block_header_t* block) {
    return (block_header_t*)((char*)block + GET_SIZE(block));
}

/* Get previous physical block (only if previous is free) */
static inline block_header_t* get_prev_block(block_header_t* block) {
    block_footer_t* prev_footer = (block_footer_t*)((char*)block - FOOTER_SIZE);
    return get_header_from_footer(prev_footer);
}

/* Global state */
static block_header_t* free_lists[NUM_SIZE_CLASSES];
static int heap_initialized = 0;
static char* heap_start = NULL;
static char* heap_end = NULL;



/*
//unncomment these if there is a issue 



void malloc_reset(void) {
    for (int i = 0; i < NUM_SIZE_CLASSES; i++) {
        free_lists[i] = NULL;
    }
    for (int i = 0; i < MAX_ARENAS; i++) {
        arenas[i].active = 0;
    }
    arena_count = 0;
    heap_initialized = 0;
    heap_start = NULL;
    heap_end = NULL;
    sbrk_current_brk = NULL;
    sbrk_current_arena = NULL;
    malloc_call_count = 0;
}

//these were deleted but can cause some errors if nonexisten so just delete these command guys to yknow put em back

*/


/* Forward declare write for debug output */
extern int write(int fd, const void* buf, unsigned int count);

/* Detect context: kernel vs userspace */
#ifdef __KERNEL__
/* Kernel context - use kmalloc directly */
extern void* kmalloc(uint32_t size);
extern void kfree(void* ptr);
#define USE_KMALLOC 1
#else
/* Userspace context - use brk syscall */
#define USE_KMALLOC 0
#endif

/* Arena management - track large chunks allocated from backing store */
#define MAX_ARENAS 64
typedef struct arena {
    char* start;
    char* end;
    size_t size;
    int active;
} arena_t;

static arena_t arenas[MAX_ARENAS];
static int arena_count = 0;

/* Check if pointer belongs to any arena */
static arena_t* find_arena(void* ptr) {
    char* p = (char*)ptr;
    for (int i = 0; i < arena_count; i++) {
        if (arenas[i].active && p >= arenas[i].start && p < arenas[i].end) {
            return &arenas[i];
        }
    }
    return NULL;
}

/* Simple brk syscall wrapper */
static int32_t sys_brk(void* addr) {
    int32_t result;
    __asm__ volatile(
        "int $0x80"
        : "=a"(result)
        : "a"(45), "b"(addr)
        : "memory"
    );
    return result;
}

/* Allocate new arena from backing store (kmalloc or brk syscall) */
static arena_t* allocate_arena(size_t min_size) {
    if (arena_count >= MAX_ARENAS) {
        write(1, "[malloc] Arena limit reached!\n", 30);
        return NULL;
    }
    
    /* Round up to nice chunk size (at least 1MB) */
    size_t arena_size = min_size;
    if (arena_size < 1024 * 1024) {
        arena_size = 1024 * 1024;
    }
    /* Round up to next power of 2 for large allocations */
    if (arena_size > 1024 * 1024) {
        size_t rounded = 1024 * 1024;
        while (rounded < arena_size) {
            rounded *= 2;
        }
        arena_size = rounded;
    }
    
    void* mem;
    
#if USE_KMALLOC
    /* Kernel mode: use kmalloc */
    mem = kmalloc((uint32_t)arena_size);
    if (!mem) {
        write(1, "[malloc] kmalloc failed for arena\n", 34);
        return NULL;
    }
#else
    /* Userspace mode: use brk syscall */
    /* First, trigger allocation by requesting memory directly */
    uint32_t new_brk = (uint32_t)sys_brk(0) + arena_size;
    int32_t result = sys_brk((void*)new_brk);

    if (result != (int32_t)new_brk) {
        write(1, "[malloc] brk expansion failed\n", 30);
        return NULL;
    }

    /* mem starts at result - arena_size, NOT at the query result */
    mem = (void*)(result - arena_size);
#endif
    
    /* Register arena */
    arena_t* arena = &arenas[arena_count++];
    arena->start = (char*)mem;
    arena->end = (char*)mem + arena_size;
    arena->size = arena_size;
    arena->active = 1;
    
    /* Update global heap_end to track the highest address */
    if (arena->end > heap_end) {
        heap_end = arena->end;
    }
    
    /* Debug output */
    write(1, "[malloc] New arena: ", 20);
    char buf[32];
    uint32_t mb = arena_size / (1024 * 1024);
    int bp = 0;
    if (mb == 0) buf[bp++] = '0';
    else {
        char tmp[16];
        int tp = 0;
        while (mb > 0) { tmp[tp++] = '0' + (mb % 10); mb /= 10; }
        while (tp > 0) buf[bp++] = tmp[--tp];
    }
    write(1, buf, bp);
    write(1, "MB\n", 3);
    
    return arena;
}

/* sbrk state - reset between processes */
static char* sbrk_current_brk = NULL;
static arena_t* sbrk_current_arena = NULL;

/* sbrk replacement - manages arenas backed by kmalloc */
static void* sbrk(intptr_t increment) {
    
    if (increment == 0) {
        /* Query current brk */
        return sbrk_current_brk ? sbrk_current_brk : (void*)0;
    }
    
    if (increment < 0) {
        /* Shrinking not supported in this implementation */
        return (void*)-1;
    }
    
    /* Check if current arena has space */
    if (sbrk_current_arena && sbrk_current_brk + increment <= sbrk_current_arena->end) {
        char* old_brk = sbrk_current_brk;
        sbrk_current_brk += increment;
        return old_brk;
    }
    
    /* Need new arena */
    sbrk_current_arena = allocate_arena((size_t)increment);
    if (!sbrk_current_arena) {
        return NULL;
    }
    
    /* Initialize brk in new arena */
    sbrk_current_brk = sbrk_current_arena->start;
    char* old_brk = sbrk_current_brk;
    sbrk_current_brk += increment;
    
    return old_brk;
}

/* Get size class index for a given size */
static inline int get_size_class(size_t size) {
    /* Size classes: 32, 64, 96, 128, 160, ..., 512, 1024, 2048, ... */
    if (size <= 512) {
        int class = (size - 1) / 32;
        if (class >= NUM_SIZE_CLASSES) class = NUM_SIZE_CLASSES - 1;
        return class;
    } else if (size <= 4096) {
        int class = 16 + (size - 513) / 256;
        if (class >= NUM_SIZE_CLASSES) class = NUM_SIZE_CLASSES - 1;
        return class;
    } else {
        /* Large sizes - use exponential size classes */
        int class = 20;  /* Start at a reasonable base for 4KB+ */
        size_t threshold = 4096;
        while (threshold < size && class < NUM_SIZE_CLASSES - 1) {
            threshold *= 2;
            class++;
        }
        return class;
    }
}

/* Add block to appropriate free list */
static void add_to_free_list(block_header_t* block) {
    size_t size = GET_SIZE(block);
    int class = get_size_class(size);
    
    block->next = free_lists[class];
    block->prev = NULL;
    
    if (free_lists[class]) {
        free_lists[class]->prev = block;
    }
    
    free_lists[class] = block;
}

/* Remove block from free list */
static void remove_from_free_list(block_header_t* block) {
    if (block->prev) {
        block->prev->next = block->next;
    } else {
        /* Block is head of list */
        size_t size = GET_SIZE(block);
        int class = get_size_class(size);
        free_lists[class] = block->next;
    }
    
    if (block->next) {
        block->next->prev = block->prev;
    }
}

/* Coalesce block with adjacent free blocks */
static block_header_t* coalesce(block_header_t* block) {
     if (GET_SIZE(block) < MIN_BLOCK_SIZE + OVERHEAD) {
        write(1, "[malloc] CORRUPT BLOCK IN COALESCE\n", 35); // if you get this there is eine kleine issue gng 
        return block;
     }
    block_header_t* next = get_next_block(block);
    int next_allocated = ((char*)next >= heap_end) || IS_ALLOCATED(next);
    int prev_allocated = IS_PREV_ALLOCATED(block);
    
    size_t size = GET_SIZE(block);
    
    if (prev_allocated && next_allocated) {
        /* No coalescing - just update next block's prev bit */
        if ((char*)next < heap_end) {
            SET_PREV_FREE(next);
        }
        return block;
    }
    else if (prev_allocated && !next_allocated) {
        /* Coalesce with next */
        size += GET_SIZE(next);
        remove_from_free_list(next);
        SET_SIZE(block, size);
        
        block_footer_t* footer = get_footer(block);
        footer->size = size;
        
        /* Update next-next block's prev bit */
        block_header_t* next_next = get_next_block(block);
        if ((char*)next_next < heap_end) {
            SET_PREV_FREE(next_next);
        }
        
        return block;
    }
    else if (!prev_allocated && next_allocated) {
        /* Coalesce with previous */
        block_header_t* prev = get_prev_block(block);
        size += GET_SIZE(prev);
        
        remove_from_free_list(prev);
        SET_SIZE(prev, size);
        
        block_footer_t* footer = get_footer(prev);
        footer->size = size;
        
        /* Update next block's prev bit */
        if ((char*)next < heap_end) {
            SET_PREV_FREE(next);
        }
        
        return prev;
    }
    else {
        /* Coalesce with both */
        block_header_t* prev = get_prev_block(block);
        size += GET_SIZE(prev) + GET_SIZE(next);
        
        remove_from_free_list(prev);
        remove_from_free_list(next);
        SET_SIZE(prev, size);
        
        block_footer_t* footer = get_footer(prev);
        footer->size = size;
        
        /* Update next-next block's prev bit */
        block_header_t* next_next = get_next_block(prev);
        if ((char*)next_next < heap_end) {
            SET_PREV_FREE(next_next);
        }
        
        return prev;
    }
}

/* Split block if it's too large */
static void split_block(block_header_t* block, size_t size) {
    size_t block_size = GET_SIZE(block);
    size_t remainder = block_size - size;
    
    /* Only split if remainder is large enough */
    if (remainder >= MIN_BLOCK_SIZE + OVERHEAD) {
        /* Shrink current block */
        SET_SIZE(block, size);
        SET_ALLOCATED(block);
        /* keep footer valid for allocated block too */
        block_footer_t* block_footer = get_footer(block);
        block_footer->size = size;
        
        /* Create new free block from remainder */
        block_header_t* new_block = get_next_block(block);
        SET_SIZE(new_block, remainder);
        SET_FREE(new_block);
        SET_PREV_ALLOCATED(new_block);
        
        block_footer_t* footer = get_footer(new_block);
        footer->size = remainder;
        
        /* Mark next block's prev_allocated bit */
        block_header_t* next = get_next_block(new_block);
        if ((char*)next < heap_end) {
            SET_PREV_FREE(next);
        }
        
        add_to_free_list(new_block);
    } else {
        /* Use entire block */
        SET_ALLOCATED(block);
        
         // i will show my foot in your footer 
         block_footer_t* footer = get_footer(block);
         footer->size = GET_SIZE(block);

        // mark my nüttesack
        block_header_t* next = get_next_block(block);
        if ((char*)next < heap_end) {
            SET_PREV_ALLOCATED(next);
        }
    }
}

/* Initialize the heap */
/* Initialize the heap */
static void heap_init(void) {
    /* Initialize all free lists to NULL */
    for (int i = 0; i < NUM_SIZE_CLASSES; i++) {
        free_lists[i] = NULL;
    }
    
    /* Initialize arena tracking */
    for (int i = 0; i < MAX_ARENAS; i++) {
        arenas[i].active = 0;
    }
    
    /* Get initial heap via sbrk (which will allocate first arena from backing store) */
    write(1, "[malloc] Initializing segregated free list allocator...\n", 57);
    
    /* Try progressively smaller initial heap sizes */
    uint32_t sizes[] = {
        64 * 1024 * 1024,   // 64MB
        32 * 1024 * 1024,   // 32MB
        16 * 1024 * 1024,   // 16MB
        8 * 1024 * 1024,    // 8MB
        4 * 1024 * 1024,    // 4MB
        2 * 1024 * 1024,    // 2MB
        1024 * 1024,        // 1MB
    };
    
    int success = 0;
    for (int i = 0; i < 7; i++) {
        write(1, "[malloc] Trying ", 16);
        uint32_t mb = sizes[i] / (1024 * 1024);
        char buf[16];
        int bp = 0;
        if (mb == 0) buf[bp++] = '0';
        else {
            char tmp[16];
            int tp = 0;
            while (mb > 0) { tmp[tp++] = '0' + (mb % 10); mb /= 10; }
            while (tp > 0) buf[bp++] = tmp[--tp];
        }
        write(1, buf, bp);
        write(1, "MB initial heap...\n", 19);

        /* Call allocate_arena directly - it handles the brk logic */
        arena_t* arena = allocate_arena(sizes[i]);
        if (arena) {
            write(1, "[malloc] SUCCESS! Segregated allocator ready.\n", 47);

            uintptr_t addr = (uintptr_t)arena->start;
            uint32_t misalign = addr & 15;
            if (misalign) {
                addr += (16 - misalign);  // push forward to next 16-byte boundary
                write(1, "[malloc] Aligned to 16\n", 23);
            }

            heap_start = (char*)addr;
            heap_end = arena->end;

            /* Create initial free block spanning entire heap */
            block_header_t* initial = (block_header_t*)heap_start;
            size_t initial_size = heap_end - heap_start;

            initial->size = 0;              /* clear flags first */
            SET_SIZE(initial, initial_size);
            SET_FREE(initial);
            SET_PREV_ALLOCATED(initial);

            block_footer_t* footer = get_footer(initial);
            footer->size = initial_size;

            add_to_free_list(initial);

            heap_initialized = 1;
            success = 1;
            break;
        } else {
            write(1, "[malloc] Failed, trying smaller size...\n", 40);
        }
    }
    
    if (!success) {
        write(1, "[malloc] FATAL: Could not initialize heap!\n", 44);
        heap_start = NULL;
    }
}

/* malloc call counter - file scope so malloc_reset can clear it */
static int malloc_call_count = 0;

/* Reset allocator state - called on process exit so next process starts fresh */
void malloc_reset(void) {
    for (int i = 0; i < NUM_SIZE_CLASSES; i++) {
        free_lists[i] = NULL;
    }
    for (int i = 0; i < MAX_ARENAS; i++) {
        arenas[i].active = 0;
    }
    arena_count = 0;
    heap_initialized = 0;
    heap_start = NULL;
    heap_end = NULL;
    sbrk_current_brk = NULL;
    sbrk_current_arena = NULL;
    malloc_call_count = 0;
}

/* Allocate memory */
void* malloc(size_t size) {
    malloc_call_count++;
    
    if (malloc_call_count % 100 == 0) {
        write(1, "[malloc] call #", 15);
        char buf[16];
        int bp = 0;
        int tmp = malloc_call_count;
        if (tmp == 0) buf[bp++] = '0';
        else {
            char rev[16];
            int rp = 0;
            while (tmp > 0) { rev[rp++] = '0' + (tmp % 10); tmp /= 10; }
            while (rp > 0) buf[bp++] = rev[--rp];
        }
        write(1, buf, bp);
        write(1, "\n", 1);
    }
    
    if (size == 0) return NULL;
    if (!heap_initialized) heap_init();
    
    if (!heap_initialized || !heap_start) {
        write(1, "[malloc] ERROR: heap not initialized!\n", 38);
        return NULL;
    }
    
    /* Align size and add overhead */
    size = ALIGN(size);
    if (size < MIN_BLOCK_SIZE) {
        size = MIN_BLOCK_SIZE;
    }
    /* Free blocks need header + footer, so total size includes both */
    size_t total_size = size + HEADER_SIZE + FOOTER_SIZE;

    /* Search for suitable block starting from appropriate size class */
    int start_class = get_size_class(total_size);

    for (int class = start_class; class < NUM_SIZE_CLASSES; class++) {
        block_header_t* current = free_lists[class];

        /* Best-fit search within this size class */
        block_header_t* best_fit = NULL;
        size_t best_size = SIZE_MAX;

        while (current) {
            size_t current_size = GET_SIZE(current);
            if (current_size >= total_size && current_size < best_size) {
                best_fit = current;
                best_size = current_size;
                if (current_size == total_size)
                    break;
            }
            current = current->next;
        }

        if (best_fit) {
            if (GET_SIZE(best_fit) < MIN_BLOCK_SIZE + OVERHEAD) {
                write(1, "[malloc] CORRUPT BEST_FIT BLOCK\n", 32);
                return NULL;
            }
            remove_from_free_list(best_fit);
            split_block(best_fit, total_size);
            return (void*)((char*)best_fit + HEADER_SIZE);
        }
    }

    /* No suitable block found */
    return NULL;
}

/* Free memory */
void free(void* ptr) {
    if (!ptr) return;
    
    /* Get block header */
    block_header_t* block = (block_header_t*)((char*)ptr - HEADER_SIZE);

    /* sanity check */
    if (GET_SIZE(block) < MIN_BLOCK_SIZE + OVERHEAD) {
        write(1, "[malloc] FREE CORRUPT BLOCK\n", 28);
        return;
    }

    /* Mark as free */
    SET_FREE(block);
    
    /* Set footer */
    block_footer_t* footer = get_footer(block);
    footer->size = GET_SIZE(block);
    
    /* Mark next block's prev_allocated bit */
    block_header_t* next = get_next_block(block);
    if ((char*)next < heap_end) {
        SET_PREV_FREE(next);
    }
    
    /* Coalesce with adjacent free blocks */
    block = coalesce(block);
    
    /* Add to appropriate free list */
    add_to_free_list(block);
}

/* Allocate and zero memory */
void* calloc(size_t nmemb, size_t size) {
    /* Check for overflow */
    if (nmemb != 0 && size > SIZE_MAX / nmemb) {
        return NULL;
    }
    
    size_t total = nmemb * size;
    void* ptr = malloc(total);
    
    if (ptr) {
        /* Zero memory */
        char* p = (char*)ptr;
        for (size_t i = 0; i < total; i++) {
            p[i] = 0;
        }
    }
    
    return ptr;
}

/* Reallocate memory */
void* realloc(void* ptr, size_t size) {
    if (!ptr) return malloc(size);
    
    if (size == 0) {
        free(ptr);
        return NULL;
    }
    
    /* Get current block */
    block_header_t* block = (block_header_t*)((char*)ptr - HEADER_SIZE);
    size_t old_size = GET_SIZE(block) - HEADER_SIZE - FOOTER_SIZE;
    
    /* If new size fits in current block, return it */
    if (size <= old_size) {
        return ptr;
    }
    
    /* Try to expand in place by coalescing with next block */
    block_header_t* next = get_next_block(block);
    if ((char*)next < heap_end && !IS_ALLOCATED(next)) {
        size_t combined_size = GET_SIZE(block) + GET_SIZE(next);
        size_t needed = ALIGN(size) + HEADER_SIZE + FOOTER_SIZE;
        
        if (combined_size >= needed) {
            /* Expand in place */
            remove_from_free_list(next);
            SET_SIZE(block, combined_size);
            
            /* Set footer for the merged block before splitting */
            block_footer_t* footer = get_footer(block);
            footer->size = combined_size;
            
            split_block(block, needed);
            return ptr;
        }
    }
    
    /* Allocate new block and copy data */
    void* new_ptr = malloc(size);
    if (!new_ptr) return NULL;
    
    /* Copy old data */
    char* src = (char*)ptr;
    char* dst = (char*)new_ptr;
    for (size_t i = 0; i < old_size; i++) {
        dst[i] = src[i];
    }

    free(ptr);
    return new_ptr;
}


// dma allocatoru 

void* dma_alloc(unsigned int size){

    dma_ptr = (dma_ptr + 31) & ~31; // 32 bite alligneledim HEHEHEHEH 31 HEHE COKGOMIK DIMI
    void* adres = (void*)dma_ptr;
    dma_ptr += size;
    return adres;
}