#include "interrupts.h"
#include "vga.h"
#include "syscall.h"

#define IDT_ENTRIES 256
#define PIC1_COMMAND 0x20
#define PIC1_DATA 0x21
#define PIC2_COMMAND 0xA0
#define PIC2_DATA 0xA1

struct idt_entry idt[IDT_ENTRIES];
struct idt_ptr idtp;

// PIC remap
void pic_init() {
    // ICW1
    __asm__ volatile("outb %%al, %%dx" : : "a"(0x11), "d"(PIC1_COMMAND));
    __asm__ volatile("outb %%al, %%dx" : : "a"(0x11), "d"(PIC2_COMMAND));
    
    // ICW2
    __asm__ volatile("outb %%al, %%dx" : : "a"(0x20), "d"(PIC1_DATA));
    __asm__ volatile("outb %%al, %%dx" : : "a"(0x28), "d"(PIC2_DATA));
    
    // ICW3
    __asm__ volatile("outb %%al, %%dx" : : "a"(0x04), "d"(PIC1_DATA));
    __asm__ volatile("outb %%al, %%dx" : : "a"(0x02), "d"(PIC2_DATA));
    
    // ICW4
    __asm__ volatile("outb %%al, %%dx" : : "a"(0x01), "d"(PIC1_DATA));
    __asm__ volatile("outb %%al, %%dx" : : "a"(0x01), "d"(PIC2_DATA));
    
    // Mask all interrupts
    __asm__ volatile("outb %%al, %%dx" : : "a"(0xFF), "d"(PIC1_DATA));
    __asm__ volatile("outb %%al, %%dx" : : "a"(0xFF), "d"(PIC2_DATA));
}

void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags) {
    idt[num].base_lo = (base & 0xFFFF);
    idt[num].base_hi = (base >> 16) & 0xFFFF;
    idt[num].sel = sel;
    idt[num].always0 = 0;
    idt[num].flags = flags;
}

void interrupts_init() {
    idtp.limit = (sizeof(struct idt_entry) * IDT_ENTRIES) - 1;
    idtp.base = (uint32_t)&idt;
    
    // IDT'yi temizle
    for (int i = 0; i < IDT_ENTRIES; i++) {
        idt_set_gate(i, 0, 0x08, 0x8E);
    }
    
    // ISR'ları ayarla
    idt_set_gate(0, (uint32_t)isr0, 0x08, 0x8E);
    idt_set_gate(1, (uint32_t)isr1, 0x08, 0x8E);
    idt_set_gate(2, (uint32_t)isr2, 0x08, 0x8E);
    idt_set_gate(3, (uint32_t)isr3, 0x08, 0x8E);
    idt_set_gate(4, (uint32_t)isr4, 0x08, 0x8E);
    idt_set_gate(5, (uint32_t)isr5, 0x08, 0x8E);
    idt_set_gate(6, (uint32_t)isr6, 0x08, 0x8E);
    idt_set_gate(7, (uint32_t)isr7, 0x08, 0x8E);
    idt_set_gate(8, (uint32_t)isr8, 0x08, 0x8E);
    idt_set_gate(9, (uint32_t)isr9, 0x08, 0x8E);
    idt_set_gate(10, (uint32_t)isr10, 0x08, 0x8E);
    idt_set_gate(11, (uint32_t)isr11, 0x08, 0x8E);
    idt_set_gate(12, (uint32_t)isr12, 0x08, 0x8E);
    idt_set_gate(13, (uint32_t)isr13, 0x08, 0x8E);
    idt_set_gate(14, (uint32_t)isr14, 0x08, 0x8E);
    idt_set_gate(15, (uint32_t)isr15, 0x08, 0x8E);
    idt_set_gate(16, (uint32_t)isr16, 0x08, 0x8E);
    idt_set_gate(17, (uint32_t)isr17, 0x08, 0x8E);
    idt_set_gate(18, (uint32_t)isr18, 0x08, 0x8E);
    idt_set_gate(19, (uint32_t)isr19, 0x08, 0x8E);
    idt_set_gate(20, (uint32_t)isr20, 0x08, 0x8E);
    idt_set_gate(21, (uint32_t)isr21, 0x08, 0x8E);
    idt_set_gate(22, (uint32_t)isr22, 0x08, 0x8E);
    idt_set_gate(23, (uint32_t)isr23, 0x08, 0x8E);
    idt_set_gate(24, (uint32_t)isr24, 0x08, 0x8E);
    idt_set_gate(25, (uint32_t)isr25, 0x08, 0x8E);
    idt_set_gate(26, (uint32_t)isr26, 0x08, 0x8E);
    idt_set_gate(27, (uint32_t)isr27, 0x08, 0x8E);
    idt_set_gate(28, (uint32_t)isr28, 0x08, 0x8E);
    idt_set_gate(29, (uint32_t)isr29, 0x08, 0x8E);
    idt_set_gate(30, (uint32_t)isr30, 0x08, 0x8E);
    idt_set_gate(31, (uint32_t)isr31, 0x08, 0x8E);
    idt_set_gate(128, (uint32_t)isr128, 0x08, 0xEE);
    
    // PIC'i remap et
    pic_init();
    
    // IDT'yi yükle
    __asm__ volatile("lidt %0" : : "m"(idtp));
    
    // Interrupt'ları etkinleştir
    __asm__ volatile("sti");
}

void pic_send_eoi(uint8_t irq) {
    if (irq >= 8) {
        // PIC2'ye EOI gönder
        __asm__ volatile("outb %%al, %%dx" : : "a"(0x20), "d"(PIC2_COMMAND));
    }
    // PIC1'e EOI gönder (her zaman)
    __asm__ volatile("outb %%al, %%dx" : : "a"(0x20), "d"(PIC1_COMMAND));
}

// External variables from elf.c for program exit handling
extern uint32_t saved_kernel_esp;
extern uint32_t saved_kernel_ebp;
extern int program_exit_requested;

// ISR handler
void isr_handler(struct regs* r) {
    // Debug: Print MY BALLSACKS CURRENT STATUS
    if (r->int_no == 128) {
        // CRITICAL: Print immediately to verify int 0x80 is firing
        print_color("\n!!! SYSCALL FIRED !!! eax=", VGA_COLOR_LIGHT_GREEN);
        // Print eax value
        char eax_hex[16];
        int eax_pos = 0;
        uint32_t eax_val = r->eax;
        for (int i = 7; i >= 0; i--) {
            uint8_t nibble = (eax_val >> (i * 4)) & 0xF;
            eax_hex[eax_pos++] = (nibble < 10) ? ('0' + nibble) : ('a' + nibble - 10);
        }
        eax_hex[eax_pos] = '\0';
        print(eax_hex);
        print("]\n");
        
        // Linux syscall (int 0x80)
        // Linux syscall convention: eax = syscall number, ebx, ecx, edx, esi, edi, ebp = args
        int32_t result = handle_syscall(r->eax, r->ebx, r->ecx, r->edx, r->esi, r->edi, r->ebp);
        r->eax = result;  // Return value in eax
        
        // Check if program requested exit
        if (program_exit_requested && saved_kernel_esp != 0 && saved_kernel_ebp != 0) {
            extern void elf_exit_handler_asm();
            extern uint32_t saved_kernel_esp_for_exit;
            extern uint32_t saved_kernel_ebp_for_exit;
            
            print_color("[EXIT REQUESTED] Modifying return EIP\n", VGA_COLOR_LIGHT_GREEN);
            
            // Use inline assembly to directly access the stack and modify EIP
            // The stack when isr_handler is called has the EIP pushed by int 0x80
            // We need to find it relative to the regs struct pointer
            // Stack: [call ret][int_no][err][ds][edi][esi][ebp][esp][ebx][edx][ecx][eax][EIP][CS][EFLAGS]
            // r points to edi, so EIP is at r - 44 bytes (8 regs before edi + ds + err + int_no + call_ret)
            uint32_t old_eip;
            __asm__ volatile(
                "mov %1, %%eax\n\t"      // Get pointer to regs struct
                "sub $44, %%eax\n\t"     // Go back 44 bytes to find EIP
                "mov (%%eax), %%ebx\n\t" // Read old EIP
                "mov %%ebx, %0\n\t"     // Store in old_eip
                "mov %2, %%ecx\n\t"     // Get exit handler address
                "mov %%ecx, (%%eax)"    // Write new EIP
                : "=r"(old_eip)
                : "r"((uint32_t)r), "r"((uint32_t)elf_exit_handler_asm)
                : "eax", "ebx", "ecx", "memory"
            );
            
            print_color("[EXIT] Old EIP: 0x", VGA_COLOR_LIGHT_GREEN);
            // Print old EIP
            char eip_hex[16];
            int eip_pos = 0;
            for (int i = 7; i >= 0; i--) {
                uint8_t nibble = (old_eip >> (i * 4)) & 0xF;
                eip_hex[eip_pos++] = (nibble < 10) ? ('0' + nibble) : ('a' + nibble - 10);
            }
            eip_hex[eip_pos] = '\0';
            print(eip_hex);
            print("\n");
            
            // Save kernel stack values for exit handler
            saved_kernel_esp_for_exit = saved_kernel_esp;
            saved_kernel_ebp_for_exit = saved_kernel_ebp;
            
            // Clear saved pointers
            saved_kernel_esp = 0;
            saved_kernel_ebp = 0;
            program_exit_requested = 0;
            
            print_color("[EXIT] EIP modified, stack will be restored on iret\n", VGA_COLOR_LIGHT_GREEN);
        }
        
        return;
    }
    
    // Debug: Print ANY interrupt during program execution (including faults)
    extern void* elf_exit_label_addr;
    if (elf_exit_label_addr != 0 && r->int_no != 128) {
        print_color("\n!!! FAULT/INTERRUPT DURING PROGRAM: ", VGA_COLOR_LIGHT_RED);
        char int_buf[8];
        int int_pos = 0;
        int int_num = r->int_no;
        if (int_num == 0) {
            int_buf[int_pos++] = '0';
        } else {
            char digits[8];
            int dpos = 0;
            while (int_num > 0 && dpos < 7) {
                digits[dpos++] = '0' + (int_num % 10);
                int_num /= 10;
            }
            for (int i = dpos - 1; i >= 0; i--) {
                int_buf[int_pos++] = digits[i];
            }
        }
        int_buf[int_pos] = '\0';
        print(int_buf);
        print(" during program execution]\n");
        while(1) { __asm__ volatile("cli; hlt"); }
    }
    
    // Catch faults during program execution to prevent reboot
    if (elf_exit_label_addr != 0 && saved_kernel_esp != 0 && saved_kernel_ebp != 0) {
        // Fault during program execution - restore stack and return to shell
        // Print debug message first (before accessing any more memory)
        print_color("\n!!! CRITICAL FAULT DURING PROGRAM !!!\n", VGA_COLOR_LIGHT_RED);
        print_color("Fault number: ", VGA_COLOR_LIGHT_RED);
        // Simple int to string for fault number
        char fault_buf[16];
        int fault_num = r->int_no;
        int fpos = 0;
        if (fault_num == 0) {
            fault_buf[fpos++] = '0';
        } else {
            char digits[16];
            int dpos = 0;
            while (fault_num > 0 && dpos < 15) {
                digits[dpos++] = '0' + (fault_num % 10);
                fault_num /= 10;
            }
            for (int i = dpos - 1; i >= 0; i--) {
                fault_buf[fpos++] = digits[i];
            }
        }
        fault_buf[fpos] = '\0';
        print(fault_buf);
        print("\n");
        
        extern void elf_fault_recovery();
        extern uint32_t saved_kernel_esp_for_exit;
        extern uint32_t saved_kernel_ebp_for_exit;
        
        saved_kernel_esp_for_exit = saved_kernel_esp;
        saved_kernel_ebp_for_exit = saved_kernel_ebp;
        saved_kernel_esp = 0;
        saved_kernel_ebp = 0;
        
        uint32_t* eip_ptr = (uint32_t*)((uint32_t)r - 12);
        *eip_ptr = (uint32_t)elf_fault_recovery;
        
        return;
    }
    
    // Basit interrupt handler - sadece ekrana yazdır
    uint16_t* vga = (uint16_t*)0xB8000;
    vga[80] = 0x0F49; // I
    vga[81] = 0x0F4E; // N
    vga[82] = 0x0F54; // T
    vga[83] = 0x0F20; // space
    vga[84] = 0x0F30 + (r->int_no % 10); // interrupt number
    
    pic_send_eoi(r->int_no);
} 
