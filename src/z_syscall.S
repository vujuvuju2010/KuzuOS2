section .text
global z_syscall

z_syscall:
	; Preserve ABI required registers.
	push	ebp
	push	edi
	push	esi
	push	ebx
	; Move arguments to registers.	
	mov	ebp, [esp + 44]
	mov	edi, [esp + 40]
	mov	esi, [esp + 36]
	mov	edx, [esp + 32]
	mov	ecx, [esp + 28]
	mov	ebx, [esp + 24]
	mov	eax, [esp + 20]	; syscall number
	; Jump to kernel, return value comes to eax.
	int	0x80
	; Restore preserved registers.
	pop	ebx
	pop	esi
	pop	edi
	pop	ebp
	ret

