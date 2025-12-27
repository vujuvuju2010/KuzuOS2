section .text
global z_trampo

z_trampo:
        ; Arguments: entry point, stack pointer, fini (unused)
        mov     eax, [esp + 4]   ; entry point
        mov     ecx, [esp + 8]   ; user stack pointer
        ; edx = fini (ignored)
        
        ; We need to switch to Ring 3 (user mode) before jumping
        ; Use iret to switch modes properly
        
        ; Set up user data segment (Ring 3)
        mov     ax, 0x23         ; User data segment (GDT entry 4, RPL=3)
        mov     ds, ax
        mov     es, ax
        mov     fs, ax
        mov     gs, ax
        
        ; Push values for iret to switch to Ring 3
        ; Stack for iret: SS, ESP, EFLAGS, CS, EIP
        
        push    0x23             ; User data segment (SS)
        push    ecx              ; User stack (ESP)
        pushf                    ; Current EFLAGS
        pop     edx
        or      edx, 0x200       ; Set IF (enable interrupts)
        push    edx              ; EFLAGS with interrupts enabled
        push    0x1B             ; User code segment (GDT entry 3, RPL=3)
        push    eax              ; Entry point (EIP)
        
        iret                     ; Jump to user mode!
        
        ; Should not reach
        hlt
