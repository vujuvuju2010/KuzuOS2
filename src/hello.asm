; Hello World assembly program for KuzuOS
; Uses Linux syscalls (int 0x80)
; Syscall numbers: SYS_WRITE=4, SYS_EXIT=1

section .data
    msg db 'Hello, World!', 0x0A  ; Message with newline
    msg_len equ $ - msg            ; Calculate message length

section .text
    global _start

_start:
    ; Write to stdout (fd=1)
    mov eax, 4      ; SYS_WRITE
    mov ebx, 1      ; File descriptor (stdout)
    mov ecx, msg    ; Pointer to message
    mov edx, msg_len ; Message length
    int 0x80        ; Call kernel

    ; Exit program
    mov eax, 1      ; SYS_EXIT
    mov ebx, 0      ; Exit code
    int 0x80        ; Call kernel

