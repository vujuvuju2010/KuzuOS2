; context_switch.asm
; Functions for saving and restoring CPU context (x64)

[bits 64]

; void context_save(struct cpu_context* ctx);
; Save current CPU state to context structure
global context_save
context_save:
    ; rdi = pointer to context structure (first parameter in x64 calling convention)
    
    ; Save general purpose registers in order matching struct cpu_context
    ; r15, r14, r13, r12, r11, r10, r9, r8, rdi, rsi, rbp, rsp, rbx, rdx, rcx, rax
    mov [rdi + 0], r15    ; ctx->r15
    mov [rdi + 8], r14    ; ctx->r14
    mov [rdi + 16], r13   ; ctx->r13
    mov [rdi + 24], r12   ; ctx->r12
    mov [rdi + 32], r11   ; ctx->r11
    mov [rdi + 40], r10   ; ctx->r10
    mov [rdi + 48], r9    ; ctx->r9
    mov [rdi + 56], r8    ; ctx->r8
    
    ; Save rdi (we're using it, so save it now)
    mov rax, rdi
    mov [rdi + 64], rax   ; ctx->rdi
    
    mov [rdi + 72], rsi   ; ctx->rsi
    mov [rdi + 80], rbp   ; ctx->rbp
    mov [rdi + 88], rsp   ; ctx->rsp -> return-address slot on stack
    mov [rdi + 96], rbx   ; ctx->rbx
    mov [rdi + 104], rdx  ; ctx->rdx
    mov [rdi + 112], rcx  ; ctx->rcx
    mov [rdi + 120], rax  ; ctx->rax (using rax as temp)
    
    ; Save segment registers (zero-extend to 64-bit)
    mov rax, ds
    mov [rdi + 128], rax  ; ctx->ds
    mov rax, es
    mov [rdi + 136], rax  ; ctx->es
    mov rax, fs
    mov [rdi + 144], rax  ; ctx->fs
    mov rax, gs
    mov [rdi + 152], rax  ; ctx->gs
    
    ; Save control registers from stack frame
    ; RIP is the return address on stack
    mov rax, [rsp]
    mov [rdi + 160], rax  ; ctx->rip
    
    ; CS and flags - get from current state
    pushfq
    pop rax
    mov [rdi + 176], rax  ; ctx->rflags
    
    ; Kernel code/data selectors for iretq-safe restores
    mov qword [rdi + 168], 0x08   ; ctx->cs
    mov rax, [rdi + 88]
    add rax, 8
    mov [rdi + 184], rax          ; ctx->userrsp (post-return rsp)
    mov qword [rdi + 192], 0x10   ; ctx->ss
    
    ret

; void context_restore(struct cpu_context* ctx);
; Restore CPU state from context structure
global context_restore
context_restore:
    ; rdi = pointer to context structure
    mov rsi, rdi          ; Save context pointer in rsi temporarily
    
    ; Restore segment registers first
    mov rcx, [rsi + 128]  ; ctx->ds
    mov ds, cx
    mov rcx, [rsi + 136]  ; ctx->es
    mov es, cx
    mov rcx, [rsi + 144]  ; ctx->fs
    mov fs, cx
    mov rcx, [rsi + 152]  ; ctx->gs
    mov gs, cx
    
    ; Restore general purpose registers (except rdi, rsi, rsp)
    mov r15, [rsi + 0]    ; ctx->r15
    mov r14, [rsi + 8]    ; ctx->r14
    mov r13, [rsi + 16]   ; ctx->r13
    mov r12, [rsi + 24]   ; ctx->r12
    mov r11, [rsi + 32]   ; ctx->r11
    mov r10, [rsi + 40]   ; ctx->r10
    mov r9,  [rsi + 48]   ; ctx->r9
    mov r8,  [rsi + 56]   ; ctx->r8
    
    mov rbp, [rsi + 80]   ; ctx->rbp
    mov rbx, [rsi + 96]   ; ctx->rbx
    mov rdx, [rsi + 104]  ; ctx->rdx
    mov rcx, [rsi + 112]  ; ctx->rcx
    mov rax, [rsi + 120]  ; ctx->rax
    
    ; Restore rdi before rsp (need to read from [rsi + 64])
    mov rdi, [rsi + 64]   ; ctx->rdi
    
    ; Restore rsp (this changes the stack, must be done last)
    mov rsp, [rsi + 88]   ; ctx->rsp
    
    ; Restore rsi last (after we've used it to access context)
    mov rsi, [rsi + 72]   ; ctx->rsi
    
    ; Jump to saved rip (should be on stack at rsp)
    ret

; void context_switch_kernel(struct cpu_context* from, struct cpu_context* to);
; Switch from one kernel context to another
global context_switch_kernel
context_switch_kernel:
    ; rdi = from pointer
    ; rsi = to pointer
    
    ; Save rsi (to pointer) before calling context_save (which uses rdi)
    push rsi
    
    ; Save current context (rdi already has from pointer)
    call context_save
    
    ; Restore new context (get to pointer from stack)
    pop rdi  ; Put to pointer in rdi for context_restore
    call context_restore
    
    ; Should never reach here
    ret

