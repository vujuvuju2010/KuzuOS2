[bits 64]
global isr0, isr1, isr2, isr3, isr4, isr5, isr6, isr7
global isr8, isr9, isr10, isr11, isr12, isr13, isr14, isr15
global isr16, isr17, isr18, isr19, isr20, isr21, isr22, isr23
global isr24, isr25, isr26, isr27, isr28, isr29, isr30, isr31
global isr128
extern isr_handler

; For exceptions without error code:
; CPU stack: RIP, CS, RFLAGS, RSP, SS
; we push: err_code=0, int_no
%macro ISR_NOERR 1
isr%1:
    cli
    push qword 0           ; err_code (dummy)
    push qword %1          ; int_no
    jmp isr_common
%endmacro

; For exceptions with error code:
; CPU stack: err_code, RIP, CS, RFLAGS, RSP, SS
; we push: int_no (err_code already pushed by CPU)
%macro ISR_ERR 1
isr%1:
    cli
    push qword %1          ; int_no
    jmp isr_common
%endmacro

; ------------------------
; Common handler for all interrupts.
; No pusha in long mode -- push each GPR by hand, in the EXACT
; reverse order of struct regs in interrupts.h so the layout
; matches field-for-field.
;
; struct regs order (low addr -> high addr / top of stack -> bottom):
;   ds, r15, r14, r13, r12, r11, r10, r9, r8,
;   rdi, rsi, rbp, rbx, rdx, rcx, rax,
;   int_no, err_code,
;   rip, cs, rflags, rsp, ss   (from CPU)
;
; So we must PUSH in this order: rax, rcx, rdx, rbx, rbp, rsi, rdi,
; r8, r9, r10, r11, r12, r13, r14, r15, then ds -- because push puts
; the newest value at the LOWEST address, and struct field [0] (ds)
; needs to end up at the lowest address (top of stack after all pushes).
; ------------------------
isr_common:
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

    ; ds: long mode ignores segment values for addressing, but we still
    ; save whatever's in ds for struct compatibility / vestigial use
    xor rax, rax
    mov ax, ds
    push rax

    mov rdi, rsp             ; System V arg1 = pointer to struct regs

    ; long mode data segments are flat/don't need reloading like x32 did,
    ; but if you use a distinct kernel data selector, reload here:
    ; mov ax, 0x10
    ; mov ds, ax
    ; mov es, ax
    ; mov fs, ax
    ; mov gs, ax

    call isr_handler

    pop rax                  ; discard saved ds (not restoring segment regs)

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

    add rsp, 16               ; discard err_code and int_no (8 bytes each)
    sti
    iretq

; ------------------------
; ISR definitions -- identical structure/order to the x32 version
; ------------------------
ISR_NOERR 0
ISR_NOERR 1
ISR_NOERR 2
ISR_NOERR 3
ISR_NOERR 4
ISR_NOERR 5
ISR_NOERR 6
ISR_NOERR 7
ISR_ERR 8
ISR_NOERR 9
ISR_ERR 10
ISR_ERR 11
ISR_ERR 12
ISR_ERR 13
ISR_ERR 14
ISR_NOERR 15
ISR_NOERR 16
ISR_NOERR 17
ISR_NOERR 18
ISR_NOERR 19
ISR_NOERR 20
ISR_NOERR 21
ISR_NOERR 22
ISR_NOERR 23
ISR_NOERR 24
ISR_NOERR 25
ISR_NOERR 26
ISR_NOERR 27
ISR_NOERR 28
ISR_NOERR 29
ISR_NOERR 30
ISR_NOERR 31
ISR_NOERR 128
