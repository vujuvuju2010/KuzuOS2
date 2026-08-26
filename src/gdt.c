// globaly descripting my tables rn ahh
// yes twin we YES WE are descripting our tables GLOBALLYYY HAHAAAA now in x64 with styleeeeeeee hehe  
// after this point its claudes hot and juicy x64 code
//
#include "gdt.h"
// 7 entries: null, kernel code, kernel data, user code, user data,
// TSS-low, TSS-high (TSS descriptor needs 2 slots in long mode)
struct gdt_entry gdt[7];
struct gdt_ptr   gp;
struct tss_entry tss;
void gdt_set_gate(int num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran) {
    gdt[num].base_low    = (base & 0xFFFF);
    gdt[num].base_middle = (base >> 16) & 0xFF;
    gdt[num].base_high   = (base >> 24) & 0xFF;
    gdt[num].limit_low   = (limit & 0xFFFF);
    gdt[num].granularity = ((limit >> 16) & 0x0F) | (gran & 0xF0);
    gdt[num].access      = access;
}
// Writes the 16-byte TSS descriptor across GDT slots [idx] and [idx+1].
// idx must point at an even boundary with a free slot right after it.
static void gdt_set_tss_gate(int idx, uint64_t base, uint32_t limit, uint8_t access, uint8_t gran) {
    struct gdt_entry_tss* entry = (struct gdt_entry_tss*)&gdt[idx];
    entry->limit_low    = (limit & 0xFFFF);
    entry->base_low      = (base & 0xFFFF);
    entry->base_middle   = (base >> 16) & 0xFF;
    entry->access        = access;
    entry->granularity   = ((limit >> 16) & 0x0F) | (gran & 0xF0);
    entry->base_high     = (base >> 24) & 0xFF;
    entry->base_upper    = (uint32_t)(base >> 32);
    entry->reserved       = 0;
}
// tss lowkey fire
extern uint8_t ist1_stack_top[]; // from boot.asm
void tss_init(uint32_t idx, uint64_t kernel_stack_top) {
    uint64_t base  = (uint64_t)&tss;
    uint32_t limit = sizeof(tss);
    // fuck tss
    for (uint32_t i = 0; i < sizeof(tss); i++) {
        ((uint8_t*)&tss)[i] = 0;
    }
    // sytack these nutz
    tss.rsp0 = kernel_stack_top;
    tss.ist1 = (uint64_t)ist1_stack_top;
    tss.iomap_base = sizeof(tss);   // no IO permission bitmap -- points past the end
    // idx occupies TWO gdt[] slots (idx, idx+1) -- caller must leave both free
    gdt_set_tss_gate(idx, base, limit, 0x89, 0x00);
}
extern uint8_t stack64_top[]; // from boot.asm
void gdt_init(void) {
    gp.limit = (sizeof(struct gdt_entry) * 7) - 1;
    gp.base  = (uint64_t)&gdt;
    gdt_set_gate(0, 0, 0, 0, 0);
    gdt_set_gate(1, 0, 0, 0x9A, 0x20);
    gdt_set_gate(2, 0, 0, 0x92, 0x00);
    gdt_set_gate(3, 0, 0, 0xFA, 0x20);
    gdt_set_gate(4, 0, 0, 0xF2, 0x00);
    tss_init(5, (uint64_t)stack64_top);   // was 0x90000
    gdt_flush((uint64_t)&gp);
    tss_flush();
}
// Function to update TSS kernel stack (call this when switching tasks)
// NOTE: dropped the kss (stack segment) parameter -- x64 doesn't select
// stacks via a segment register the way x32 did, RSP0 alone is enough.
void tss_set_kernel_stack(uint64_t kesp) {
    tss.rsp0 = kesp;
}
// FPU (Floating Point Unit) initialization
// NOTE: x32 comment said "DO NOT enable SSE - TinyCC doesn't support it".
// You said x64 needs floats, and System V ABI REQUIRES SSE2 baseline
// (floats/doubles pass in XMM registers by calling convention) -- so
// this now enables it, assuming a real x86_64 cross-compiler, not TinyCC.
void fpu_init(void) {
    uint64_t cr0, cr4;
    // CR0: clear EM (bit2, no x87 emulation), set MP (bit1, monitor coprocessor)
    __asm__ volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 &= ~(1ULL << 2);
    cr0 |=  (1ULL << 1);
    __asm__ volatile("mov %0, %%cr0" : : "r"(cr0));
    __asm__ volatile("fninit");
    // CR4: set OSFXSR (bit9, enables SSE/SSE2 fxsave/fxrstor + SSE insns)
    // and OSXMMEXCPT (bit10, enables unmasked SIMD FP exceptions)
    __asm__ volatile("mov %%cr4, %0" : "=r"(cr4));
    cr4 |= (1ULL << 9);
    cr4 |= (1ULL << 10);
    __asm__ volatile("mov %0, %%cr4" : : "r"(cr4));
}
// done gng :3
