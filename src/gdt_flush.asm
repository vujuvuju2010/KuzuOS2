section .text
global gdt_flush
global tss_flush

gdt_flush:
    mov eax, [esp + 4]  ; Get the pointer to the GDT, passed as a parameter
    lgdt [eax]          ; Load the GDT
    
    ; Reload segment registers
    mov ax, 0x10        ; 0x10 is the offset in the GDT to our data segment
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    
    ; Far jump to reload CS
    jmp 0x08:.flush     ; 0x08 is the offset to our code segment
.flush:
    ret

tss_flush:
    mov ax, 0x28        ; TSS is at offset 0x28 (5th entry * 8)
    ltr ax              ; Load Task Register
    ret
