; YEE VESAAA JAHOY
;soo okay its round 7 am rn ofc i didnt sleep 
;AND IM PORTING THIS BOY TO X64 WOO HOOO so its 18.08.2026 rn
;fail count: 2 

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
    dd 0                     ; width - any 
    dd 0                      ; height - any
    dd 0                       ; depth - any
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

global pml4
global pdpt
global pd_low
global pd_high

global framebuffer
global fb_width  
global fb_height
global fb_pitch

framebuffer: dq 0 ; yes its dq not dd
fb_width: dd 0
fb_height: dd 0
fb_pitch: dd 0

global mb_magic_saved
global mb_addr_saved
mb_magic_saved: dd 0
mb_addr_saved: dd 0 ; for ram detection 

_start:
    mov esp, stack_top
    cli
    
    mov [mb_magic_saved], eax
    mov [mb_addr_saved], ebx

    ; Save multiboot info
    push ebx  ; multiboot info
    push eax  ; multiboot magic
    
    ; Check multiboot magic FIRST (use saved value)
    mov eax, [mb_magic_saved]
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
mov eax, [esi + 8]       ; frambuffera koy bi 32lik
mov [framebuffer], eax   
mov eax, [esi + 12] ; high 32 bits n shi
mov [framebuffer + 4], eax
mov eax, [esi + 16]      ; pi(tc)ç
mov [fb_pitch], eax
mov eax, [esi + 20]      ; THICHH OF IT
mov [fb_width], eax
mov eax, [esi + 24]      ; lloooooooooooooooooooooooooooooong boiiii
mov [fb_height], eax


.call_kernel:
    ; call big papi
    call long_mode_init ; call the long mode papi
    
    ; lik big papi
    add esp, 8
    
.halt: ;ezik 32 bit halt
    cli
    hlt
    jmp .halt

; EVERYTHING AFTER THIS IS FOR X64 THEE SHALL NOT FORGET THEE SHALL NOT FORGIVE
global stack64_bottom
global stack64_top

section .bss
align 4096
pml4:
    resb 4096
pdpt:
    resb 4096
pd_low:
    resb 4096
pd_1:
    resb 4096
pd_2:
    resb 4096
pd_high:
    resb 4096
align 16
stack64_bottom:
    resb 16384 * 4
stack64_top:

global ist1_stack_bottom
global ist1_stack_top
align 16
ist1_stack_bottom:
    resb 16384
ist1_stack_top:

section .rodata
align 8
gdt64:
    dq 0
.code: equ $ - gdt64
    dq (1<<43) | (1<<44) | (1<<47) | (1<<53)
.data: equ $ - gdt64
    dq (1<<44) | (1<<47) | (1<<41)
.pointer:
    dw $ - gdt64 - 1
    dq gdt64

section .text
bits 32

long_mode_init:
    ; Disable interrupts (already done but explicit)
    cli
    
    ; Disable NMI
    in al, 0x70
    or al, 0x80
    out 0x70, al
    
    ; Zero out page tables (now we have 6 tables: pml4, pdpt, pd_low, pd_1, pd_2, pd_high)
    mov edi, pml4
    mov ecx, 4096 * 6 / 4
    xor eax, eax
    rep stosd

    ; Create page tables to map first 4GB
    
    ; PML4[0] -> PDPT
    mov eax, pdpt
    or eax, 0b111  ; user, writable, present
    mov [pml4], eax

    ; PDPT[0] -> PD_low (map first 1GB)
    mov eax, pd_low
    or eax, 0b111  ; user, writable, present
    mov [pdpt], eax
    
    ; PDPT[1] -> PD_1 (map 1-2GB) 
    mov eax, pd_1
    or eax, 0b111  ; user, writable, present
    mov [pdpt + 1 * 8], eax
    
    ; PDPT[2] -> PD_2 (map 2-3GB)
    mov eax, pd_2
    or eax, 0b111  ; user, writable, present
    mov [pdpt + 2 * 8], eax
    
    ; PDPT[3] -> PD_high (map 3-4GB for framebuffer)
    mov eax, pd_high
    or eax, 0b111  ; user, writable, present
    mov [pdpt + 3 * 8], eax

    ; Map first 1GB (0-1GB) with 2MB huge pages - covers kernel
    mov ecx, 0
.map_low:
    mov eax, 0x200000
    mul ecx
    or eax, 0b10000111  ; huge page, user, writable, present
    mov [pd_low + ecx * 8], eax
    inc ecx
    cmp ecx, 512
    jne .map_low

    ; Map 1-2GB
    mov ecx, 0
.map_1:
    mov eax, 0x200000
    mul ecx
    add eax, 0x40000000  ; Start at 1GB
    or eax, 0b10000111  ; huge page, user, writable, present
    mov [pd_1 + ecx * 8], eax
    inc ecx
    cmp ecx, 512
    jne .map_1
    
    ; Map 2-3GB
    mov ecx, 0
.map_2:
    mov eax, 0x200000
    mul ecx
    add eax, 0x80000000  ; Start at 2GB
    or eax, 0b10000111  ; huge page, user, writable, present
    mov [pd_2 + ecx * 8], eax
    inc ecx
    cmp ecx, 512
    jne .map_2
    
    ; Map 3-4GB range for framebuffer
    mov ecx, 0
.map_fb:
    mov eax, 0x200000
    mul ecx
    add eax, 0xC0000000  ; Start at 3GB
    or eax, 0b10000111  ; huge page, user, writable, present
    mov [pd_high + ecx * 8], eax
    inc ecx
    cmp ecx, 512
    jne .map_fb

    ; Load page tables
    mov eax, pml4
    mov cr3, eax

    ; Enable PAE
    mov eax, cr4
    or eax, 1 << 5
    mov cr4, eax

    ; Enable long mode
    mov ecx, 0xC0000080
    rdmsr
    or eax, 1 << 8
    wrmsr

    ; Enable paging
    mov eax, cr0
    or eax, 1 << 31
    mov cr0, eax

    ; Load GDT and jump to 64-bit
    lgdt [gdt64.pointer]
    jmp gdt64.code:long_mode_start

bits 64
long_mode_start:
    mov ax, gdt64.data
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    mov rsp, stack64_top
    
    ; Enable SSE/FPU for real hardware
    mov rax, cr0
    and ax, 0xFFFB      ; Clear CR0.EM (bit 2) - No emulation
    or ax, 0x2          ; Set CR0.MP (bit 1) - Monitor coprocessor
    mov cr0, rax
    
    mov rax, cr4
    or ax, 3 << 9       ; Set CR4.OSFXSR and CR4.OSXMMEXCPT (bits 9-10)
    mov cr4, rax

    mov rdi, [framebuffer]
    mov esi, [fb_width]
    mov edx, [fb_height]
    mov ecx, [fb_pitch]
    
    xor edi, edi
    xor esi, esi
    mov edi, [mb_magic_saved]
    mov esi, [mb_addr_saved]

    call kernel_main

.halt64:
    cli
    hlt
    jmp .halt64
