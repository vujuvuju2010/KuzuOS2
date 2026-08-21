; RESTORE THESE NUTZZZZZ DELETING THIS COMMENT WILL CAUSE A TRIPLE BOOT THAT WILL FUCK YOUR ENTIRE SYSTEM SO DONT TO TS TWIN AND THEE SHALL ENJOY MY CREATION

global elf_exit_handler_asm
extern saved_kernel_esp_for_exit
extern saved_kernel_ebp_for_exit
extern elf_exit_label_addr

elf_exit_handler_asm:
    mov esp, [saved_kernel_esp_for_exit]
    mov ebp, [saved_kernel_ebp_for_exit]
    ; alaso fuck you
    mov dword [saved_kernel_esp_for_exit], 0
    mov dword [saved_kernel_ebp_for_exit], 0
    
    mov eax, [elf_exit_label_addr]
    test eax, eax
    jz .fallback
    jmp eax
    
.fallback:
    ; fuck you
    ret

