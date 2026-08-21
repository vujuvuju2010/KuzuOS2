[bits 64]
section .text
global gdt_flush
global tss_flush

; System V: arg1 (pointer to gdt_ptr) arrives in RDI, not on the stack
gdt_flush:
    lgdt [rdi]           ; load the GDT (64-bit base now, handled by gdt_ptr)

    ; Reload segment registers -- data segments are flat in long mode,
    ; this mostly just satisfies the CPU/keeps things consistent
    mov ax, 0x10          ; 0x10 = kernel data segment selector
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ; Reloading CS can't be done with a direct far jmp using a label
    ; the way x32 did it here -- push a far return frame and use retfq
    ; instead, which is the standard way to reload CS in long mode.
    push qword 0x08        ; kernel code segment selector
    lea rax, [rel .flush]
    push rax
    retfq
.flush:
    ret

tss_flush:
    mov ax, 0x28           ; TSS selector (5th entry * 8 = 0x28), unchanged
    ltr ax
    ret
