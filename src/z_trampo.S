section .text
global z_trampo
extern enter_user_mode
z_trampo:
        ; z_trampo(entry_point, user_stack_ptr, fini)
        ; x86-64 System V ABI: entry_point=rdi, user_stack_ptr=rsi, fini=rdx
        ; Pass these to enter_user_mode which expects the same registers
        
        ; Save fini (rdx) on stack since enter_user_mode will clobber it
        push rdx
        
        ; rdi = entry point (already in rdi)
        ; rsi = user stack pointer (already in rsi)
        ; enter_user_mode will read argc/argv from *rsi
        
        ; Call enter_user_mode (which uses iretq to switch to user mode)
        jmp enter_user_mode
