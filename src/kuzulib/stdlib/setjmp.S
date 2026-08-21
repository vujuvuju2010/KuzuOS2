; setjmp/longjmp for i386 (NASM syntax)
; jmp_buf layout (matches glibc i386):
;   [0]  ebx
;   [4]  esi
;   [8]  edi
;   [12] ebp
;   [16] esp  (caller's esp, i.e. esp+4 at point of setjmp call)
;   [20] eip  (return address = where setjmp was called from)

[BITS 32]

global setjmp
setjmp:
    mov edx, [esp+4]        ; edx = pointer to jmp_buf
    mov [edx+0],  ebx       ; save ebx
    mov [edx+4],  esi       ; save esi
    mov [edx+8],  edi       ; save edi
    mov [edx+12], ebp       ; save ebp
    lea eax, [esp+4]        ; caller's esp (after our return)
    mov [edx+16], eax       ; save esp
    mov eax, [esp]          ; return address (our caller's EIP)
    mov [edx+20], eax       ; save eip
    xor eax, eax            ; return 0
    ret

global longjmp
longjmp:
    mov edx, [esp+4]        ; edx = pointer to jmp_buf
    mov eax, [esp+8]        ; eax = val
    test eax, eax
    jnz .nonzero
    mov eax, 1              ; val==0 becomes 1
.nonzero:
    ; restore saved registers
    mov ebx, [edx+0]
    mov esi, [edx+4]
    mov edi, [edx+8]
    mov ebp, [edx+12]
    mov esp, [edx+16]       ; restore caller's stack pointer
    jmp dword [edx+20]      ; if doesnt work change it to 8 (in memory of robotex 2026 and findik)
