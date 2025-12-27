// gdt.c - Global Descriptor Table setup with TSS
// Define types since we're using -nostdinc
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;

struct gdt_entry {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t base_middle;
    uint8_t access;
    uint8_t granularity;
    uint8_t base_high;
} __attribute__((packed));

struct gdt_ptr {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

// TSS structure
struct tss_entry {
    uint32_t prev_tss;
    uint32_t esp0;      // Kernel stack pointer
    uint32_t ss0;       // Kernel stack segment
    uint32_t esp1;
    uint32_t ss1;
    uint32_t esp2;
    uint32_t ss2;
    uint32_t cr3;
    uint32_t eip;
    uint32_t eflags;
    uint32_t eax;
    uint32_t ecx;
    uint32_t edx;
    uint32_t ebx;
    uint32_t esp;
    uint32_t ebp;
    uint32_t esi;
    uint32_t edi;
    uint32_t es;
    uint32_t cs;
    uint32_t ss;
    uint32_t ds;
    uint32_t fs;
    uint32_t gs;
    uint32_t ldt;
    uint16_t trap;
    uint16_t iomap_base;
} __attribute__((packed));

// GDT with 6 entries: null, kernel code, kernel data, user code, user data, TSS
struct gdt_entry gdt[6];
struct gdt_ptr gp;
struct tss_entry tss;

extern void gdt_flush(uint32_t);
extern void tss_flush();

void gdt_set_gate(int num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran) {
    gdt[num].base_low = (base & 0xFFFF);
    gdt[num].base_middle = (base >> 16) & 0xFF;
    gdt[num].base_high = (base >> 24) & 0xFF;
    
    gdt[num].limit_low = (limit & 0xFFFF);
    gdt[num].granularity = ((limit >> 16) & 0x0F) | (gran & 0xF0);
    gdt[num].access = access;
}

// Initialize TSS
void tss_init(uint32_t idx, uint32_t kss, uint32_t kesp) {
    uint32_t base = (uint32_t)&tss;
    uint32_t limit = sizeof(tss);
    
    // Clear the TSS
    for (int i = 0; i < sizeof(tss); i++) {
        ((uint8_t*)&tss)[i] = 0;
    }
    
    // Set kernel stack
    tss.ss0 = kss;
    tss.esp0 = kesp;
    
    // Set TSS descriptor in GDT
    // Access: 0x89 = Present, Ring 0, TSS (not busy)
    gdt_set_gate(idx, base, limit, 0x89, 0x00);
}

void gdt_init() {
    gp.limit = (sizeof(struct gdt_entry) * 6) - 1;
    gp.base = (uint32_t)&gdt;
    
    // Null descriptor
    gdt_set_gate(0, 0, 0, 0, 0);
    
    // Kernel code segment (0x08)
    gdt_set_gate(1, 0, 0xFFFFFFFF, 0x9A, 0xCF);
    
    // Kernel data segment (0x10)
    gdt_set_gate(2, 0, 0xFFFFFFFF, 0x92, 0xCF);
    
    // User code segment (0x18, or 0x1B with RPL=3)
    gdt_set_gate(3, 0, 0xFFFFFFFF, 0xFA, 0xCF);
    
    // User data segment (0x20, or 0x23 with RPL=3)
    gdt_set_gate(4, 0, 0xFFFFFFFF, 0xF2, 0xCF);
    
    // TSS (0x28)
    // We need a kernel stack - use a reasonable address
    // This should be set to your kernel stack top
    tss_init(5, 0x10, 0x00090000);  // Kernel data segment, stack at 0x90000
    
    // Load the GDT
    gdt_flush((uint32_t)&gp);
    
    // Load the TSS
    tss_flush();
}

// Function to update TSS kernel stack (call this when switching tasks)
void tss_set_kernel_stack(uint32_t kss, uint32_t kesp) {
    tss.ss0 = kss;
    tss.esp0 = kesp;
}
