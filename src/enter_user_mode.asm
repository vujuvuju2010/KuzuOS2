; enter_user_mode.asm
; x64 version

[bits 64]
global enter_user_mode

enter_user_mode:
    ; Switch to user mode (ring 3) in x64
    ; rdi = entry point (RIP)
    ; rsi = user stack pointer (points to argc)
    ;
    ; Stack layout at rsi:
    ;   [rsi+0]   = argc
    ;   [rsi+8]   = argv[0] pointer
    ;   [rsi+16]  = argv[1] pointer
    ;   ...
    ;
    ; System V AMD64 ABI expects for _start:
    ;   rdi = argc
    ;   rsi = argv (char**) - pointer to the array of pointers
    
    ; Clear debug registers
    xor rax, rax
    mov dr7, rax
    mov dr6, rax

    ; Save entry point and stack pointer
    mov r8, rdi           ; r8 = entry point (for RIP)
    mov r9, rsi           ; r9 = user stack pointer (for RSP)
    
    ; Read argc from user stack: [r9] = argc
    mov rdi, [r9]         ; rdi = argc
    
    ; argv for _start should be the address where argv pointers start
    ; Stack: [argc][argv[0]][argv[1]]...
    ; So argv = r9 + 8 (points to argv[0] slot)
    mov rsi, r9
    add rsi, 8            ; rsi = &argv[0] = argv (char**)
    
    ; Push values for iretq (in reverse order):
    ; SS, RSP, RFLAGS, CS, RIP
    push 0x23             ; SS (user data segment, ring 3)
    push r9               ; RSP (user stack pointer)
    
    ; RFLAGS: IF=1 (interrupts)
    mov rax, 0x202
    push rax
    
    push 0x1B             ; CS (user code segment, ring 3)
    push r8               ; RIP (entry point)
    
    ; Jump to user mode via iretq
    ; NOTE: Do NOT set DS/ES/FS/GS here - in long mode they're ignored
    ; and setting them before iretq can cause GPF
    iretq

hang:
    cli
    hlt
    jmp hang
