; Minimal crti.o - C runtime init stub
; NASM syntax
section .init
global _init
_init:
    ; Empty init section
    ret

section .fini
global _fini
_fini:
    ; Empty fini section
    ret
