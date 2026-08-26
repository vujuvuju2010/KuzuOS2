section .text
global z_syscall

z_syscall:
	; x86-64 System V ABI calling convention (how C calls us):
	; param1=rdi, param2=rsi, param3=rdx, param4=rcx, param5=r8, param6=r9, param7=[stack]
	;
	; KuzuOS interrupt handler expects (from interrupts.c):
	; rax=syscall_num, rbx=arg1, rcx=arg2, rdx=arg3, rsi=arg4, rdi=arg5, rbp=arg6
	;
	; Our C signature: z_syscall(syscall_num, arg1, arg2, arg3, arg4, arg5, arg6)
	; So we receive:    z_syscall(rdi,        rsi,  rdx,  rcx,  r8,   r9,   [stack])
	; Need to arrange: (rax,                  rbx,  rcx,  rdx,  rsi,  rdi,  rbp)
	
	; Save registers we'll clobber
	push	rbx
	push	rbp
	
	; Move syscall number to rax
	mov	rax, rdi	; syscall_num: rdi -> rax
	
	; Now rearrange arguments (careful about conflicts!):
	mov	rbx, rsi	; arg1: rsi -> rbx
	; rcx already has arg3 from C calling convention, but kernel expects it as arg2
	; rdx already has arg2 from C, but kernel expects it as arg3
	; So we need to swap rcx and rdx!
	mov	r10, rcx	; save arg3 (rcx) temporarily
	mov	rcx, rdx	; arg2: rdx -> rcx  
	mov	rdx, r10	; arg3: (saved rcx) -> rdx
	; rsi needs arg4 (which is r8)
	mov	rsi, r8		; arg4: r8 -> rsi
	; rdi needs arg5 (which is r9)
	mov	rdi, r9		; arg5: r9 -> rdi
	; rbp needs arg6 (which is on stack)
	mov	rbp, [rsp + 16]	; arg6: stack -> rbp (after 2 pushes = 16 bytes)
	
	; Execute syscall
	int	0x80
	
	; Return value is in rax - leave it there
	
	; Restore saved registers
	pop	rbp
	pop	rbx
	ret

