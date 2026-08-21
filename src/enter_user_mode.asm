; enter_user_mode.asm
; x64 version

[bits 64]
global enter_user_mode

enter_user_mode:
    ; Switch to user mode (ring 3) in x64
    
    ; Clear debug registers
    xor rax, rax
    mov dr7, rax          ; clear DR7 (disables all hardware breakpoints)
    mov dr6, rax          ; clear DR6 status bits

    ; rdi = entry point (first parameter in x64 calling convention)
    ; rsi = user stack pointer (second parameter)
    
    ; Set up user data segments
    mov bx, 0x23          ; User data segment (RPL=3)
    mov ds, bx
    mov es, bx
    mov fs, bx
    mov gs, bx

    ; Push values for iretq (in reverse order):
    ; SS, RSP, RFLAGS, CS, RIP
    push 0x23             ; SS (user data segment, RPL=3)
    push rsi              ; RSP (user stack pointer)
    
    ; Prepare RFLAGS
    pushfq
    pop rdx
    or rdx, 0x200         ; IF=1 (enable interrupts)
    and rdx, ~0x100       ; TF=0 (disable trap flag)
    push rdx              ; RFLAGS
    
    push 0x1B             ; CS (user code segment, RPL=3)
    push rdi              ; RIP (entry point)
    
    ; Jump to user mode
    iretq

hang:
    cli
    hlt
    jmp hang
