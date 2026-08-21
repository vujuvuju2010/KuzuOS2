section .text
global z_syscall

z_syscall:
	; x86-64 System V ABI calling convention:
	; Arguments arrive in: rdi, rsi, rdx, rcx, r8, r9, [stack...]
	; We need to rearrange them for syscall convention:
	; Syscall expects: rax=syscall#, rdi=arg1, rsi=arg2, rdx=arg3, r10=arg4, r8=arg5, r9=arg6
	
	; Save registers that will be clobbered
	push	rbp
	push	rbx
	push	r10
	
	; Rearrange arguments:
	; rdi = syscall number (arg1) -> rax
	; rsi = arg2 -> rdi
	; rdx = arg3 -> rsi
	; rcx = arg4 -> rdx
	; r8  = arg5 -> r10
	; r9  = arg6 -> r8
	; [rsp+...] = arg7 -> r9
	
	mov	rax, rdi	; syscall number
	mov	rdi, rsi	; arg2 -> arg1 position
	mov	rsi, rdx	; arg3 -> arg2 position
	mov	r10, rcx	; arg4 -> r10 (syscall uses r10 instead of rcx)
	; r8 already has arg5
	; r9 already has arg6
	mov	rbp, [rsp + 24]	; arg7 from stack (after 3 pushes = 24 bytes)
	
	; Jump to kernel, return value comes to rax
	int	0x80
	
	; Restore preserved registers
	pop	r10
	pop	rbx
	pop	rbp
	ret

