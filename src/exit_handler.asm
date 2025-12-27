; RESTORE THESE NUTZZZZZ DELETING THIS COMMENT WILL CAUSE A TRIPLE BOOT THAT WILL FUCK YOUR ENTIRE SYSTEM SO DONT TO TS TWIN AND THEE SHALL ENJOY MY CREATION

global elf_exit_handler_asm
extern saved_kernel_esp_for_exit
extern saved_kernel_ebp_for_exit
extern elf_exit_label_addr

elf_exit_handler_asm:
    ; Restore kernel stack immediately
    mov esp, [saved_kernel_esp_for_exit]
    mov ebp, [saved_kernel_ebp_for_exit]
    
    ; Clear saved values
    mov dword [saved_kernel_esp_for_exit], 0
    mov dword [saved_kernel_ebp_for_exit], 0
    
    ; Jump to cleanup label if set
    mov eax, [elf_exit_label_addr]
    test eax, eax
    jz .fallback
    jmp eax
    
.fallback:
    ; If no label, just return (shouldn't happen)
    ret

