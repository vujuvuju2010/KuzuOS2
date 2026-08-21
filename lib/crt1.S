; Minimal crt1.o - C runtime startup stub for bare metal
; NASM syntax
section .text
global _start
_start:
    ; Do nothing - _start is already defined in the program
    ret
