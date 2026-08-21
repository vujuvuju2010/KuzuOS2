// sooo we have ts ig idk
//
//
//
typedef unsigned char       uint8_t;
typedef unsigned short      uint16_t;
typedef unsigned int        uint32_t;
typedef unsigned long long  uint64_t;

// Standard 8-byte GDT entry. In long mode, base/limit are IGNORED by the
// CPU for code/data segments -- segmentation is forced flat. Only
// access/granularity (specifically the L-bit for 64-bit code, and
// present/type/DPL) actually matter. base/limit fields are kept and
// zeroed for structural compatibility but do nothing.
struct gdt_entry {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t  base_middle;
    uint8_t  access;
    uint8_t  granularity;
    uint8_t  base_high;
} __attribute__((packed));

// TSS descriptor is 16 bytes in long mode (needs a full 64-bit base
// address) -- this does NOT fit in struct gdt_entry, so it gets its
// own layout. Occupies TWO consecutive 8-byte GDT slots.
struct gdt_entry_tss {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t  base_middle;
    uint8_t  access;
    uint8_t  granularity;
    uint8_t  base_high;
    uint32_t base_upper;   // bits 32-63 of TSS base
    uint32_t reserved;     // must be zero
} __attribute__((packed));

struct gdt_ptr {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed));

// x86_64 TSS. Nothing like the x32 version -- no GPR save area, no
// segment registers, no ss0/esp1/esp2-style per-ring stacks in that
// form. Just RSP0-2 (stack ptrs loaded on privilege-level change via
// interrupt/syscall) and IST1-7 (dedicated stacks for specific IDT
// vectors, referenced by idt_entry.ist).
struct tss_entry {
    uint32_t reserved0;
    uint64_t rsp0;          // stack ptr for ring 0
    uint64_t rsp1;          // stack ptr for ring 1
    uint64_t rsp2;          // stack ptr for ring 2
    uint64_t reserved1;
    uint64_t ist1;
    uint64_t ist2;
    uint64_t ist3;
    uint64_t ist4;
    uint64_t ist5;
    uint64_t ist6;
    uint64_t ist7;
    uint64_t reserved2;
    uint16_t reserved3;
    uint16_t iomap_base;
} __attribute__((packed));

void gdt_init(void);
void gdt_set_gate(int num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran);
void tss_init(uint32_t idx, uint64_t kernel_stack_top);
void tss_set_kernel_stack(uint64_t kesp);
void fpu_init(void);

extern void gdt_flush(uint64_t);
extern void tss_flush(void);
