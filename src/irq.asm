[bits 64]
; IRQ (Interrupt Request) handlers - x64 version
; Follows same pattern as isr.asm stubs

global irq0, irq1, irq2, irq3, irq4, irq5, irq6, irq7
global irq8, irq9, irq10, irq11, irq12, irq13, irq14, irq15

extern irq_handler

; IRQ macro - no error code for IRQs, so push dummy 0
%macro IRQ 1
irq%1:
    cli
    push qword 0                ; err_code (dummy)
    push qword (32 + %1)        ; int_no (IRQs are 32-47)
    jmp irq_common
%endmacro

; Common IRQ handler - matches isr_common pattern
irq_common:
    push rax
    push rcx
    push rdx
    push rbx
    push rbp
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    xor rax, rax
    mov ax, ds
    push rax

    mov rdi, rsp             ; System V arg1 = pointer to struct regs

    call irq_handler

    pop rax                  ; discard saved ds

    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rbp
    pop rbx
    pop rdx
    pop rcx
    pop rax

    add rsp, 16              ; discard err_code and int_no

    iretq                    ; 64-bit return from interrupt

; Generate all 16 IRQ stubs (IRQ0-15)
IRQ 0
IRQ 1
IRQ 2
IRQ 3
IRQ 4
IRQ 5
IRQ 6
IRQ 7
IRQ 8
IRQ 9
IRQ 10
IRQ 11
IRQ 12
IRQ 13
IRQ 14
IRQ 15 