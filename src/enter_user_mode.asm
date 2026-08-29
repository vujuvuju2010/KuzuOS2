; enter_user_mode.asm
; x64 version

[bits 64]
global enter_user_mode

enter_user_mode:
    ; Switch to user mode (ring 3) in x64
    ; rdi = entry point
    ; rsi = user stack pointer (points to argc)
    ;
    ; User stack layout (from loader_kernel.c elf_execute_internal):
    ;   [rsp+0]  = argc
    ;   [rsp+8]  = argv[0] pointer
    ;   [rsp+16] = argv[1] pointer
    ;   ...
    ;
    ; System V AMD64 ABI expects for _start/int main:
    ;   rdi = argc
    ;   rsi = argv (char**)
    
    ; Clear debug registers
    xor rax, rax
    mov dr7, rax
    mov dr6, rax

    ; Set up user data segments (ring 3)
    mov bx, 0x23
    mov ds, bx
    mov es, bx
    mov fs, bx
    mov gs, bx

    ; Save entry point and stack pointer
    mov r8, rdi           ; r8 = entry point (for RIP)
    mov r9, rsi           ; r9 = user stack pointer (for RSP)
    
    ; Read argc and argv from user stack and put in registers for _start
    mov rdi, [r9]         ; rdi = argc (1st arg for System V ABI)
    mov rsi, [r9 + 8]     ; rsi = argv (2nd arg for System V ABI)
    
    ; Push values for iretq (in reverse order):
    ; SS, RSP, RFLAGS, CS, RIP
    push 0x23             ; SS (user data segment)
    push r9               ; RSP (user stack - points to argc)
    
    ; RFLAGS with interrupts enabled
    pushfq
    pop rax
    or rax, 0x200         ; IF=1
    and rax, ~0x100       ; TF=0
    push rax
    
    push 0x1B             ; CS (user code segment)
    push r8               ; RIP (entry point)
    
    ; Jump to user mode
    ; On entry, _start will find:
    ;   rdi = argc, rsi = argv (already set above)
    ;   rsp = points to argc on stack
    iretq

hang:
    cli
    hlt
    jmp hang
