section .text
global z_trampo

z_trampo:
	mov	eax, [esp + 4]
	mov	ecx, [esp + 8]
	mov	edx, [esp + 12]
	mov	esp, ecx
	jmp	eax
	; Should not reach.
	hlt

