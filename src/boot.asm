; boot.asm - VESA graphics boot with kernel integration

section .multiboot
header_start:
    ; multiboot2 header
    dd 0xe85250d6                ; magic number (multiboot 2)
    dd 0                         ; architecture 0 (protected mode i386)
    dd header_end - header_start ; header length
    ; checksum
    dd 0x100000000 - (0xe85250d6 + 0 + (header_end - header_start))

    ; framebuffer tag
align 8
fb_tag_start:
    dw 5                        ; type = framebuffer
    dw 0                        ; flags  
    dd fb_tag_end - fb_tag_start ; size
    dd 1024                     ; width
    dd 768                      ; height
    dd 32                       ; depth
fb_tag_end:

    ; end tag
align 8
    dw 0                        ; type
    dw 0                        ; flags
    dd 8                        ; size
header_end:

section .bss
align 16
stack_bottom:
    resb 16384
stack_top:

section .text
bits 32
global _start
extern kernel_main

; Make VESA variables globally accessible to C code
global framebuffer
global fb_width  
global fb_height
global fb_pitch

framebuffer: dd 0
fb_width: dd 0
fb_height: dd 0
fb_pitch: dd 0

_start:
    mov esp, stack_top
    cli
    
    ; Save multiboot info
    push ebx  ; multiboot info
    push eax  ; multiboot magic
    
    ; Check multiboot magic
    cmp eax, 0x36d76289
    jne .call_kernel
    
    ; Find framebuffer tag
    mov esi, ebx
    add esi, 8              ; skip size and reserved
    
.find_fb:
    mov eax, [esi]          ; tag type
    test eax, eax           ; end tag?
    jz .call_kernel
    cmp eax, 8              ; framebuffer tag?
    je .found_fb
    
    ; Next tag
    mov ecx, [esi + 4]      ; tag size
    add ecx, 7
    and ecx, ~7             ; align to 8
    add esi, ecx
    jmp .find_fb
    
.found_fb:
    ; Get framebuffer info and store in global variables
    mov eax, [esi + 8]      ; framebuffer address
    mov [framebuffer], eax
    mov eax, [esi + 16]     ; pitch
    mov [fb_pitch], eax
    mov eax, [esi + 20]     ; width
    mov [fb_width], eax
    mov eax, [esi + 24]     ; height
    mov [fb_height], eax
    
.call_kernel:
    ; Call the kernel with multiboot parameters
    call kernel_main
    
    ; Clean up stack
    add esp, 8
    
.halt:
    cli
    hlt
    jmp .halt
