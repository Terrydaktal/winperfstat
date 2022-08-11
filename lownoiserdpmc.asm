.CODE

PUBLIC _lownoiserdpmc

_lownoiserdpmc PROC  ; entrypoint rcx, numparams rdx, outputbuffer r8, countbuffer r9
	push rsi
	push rbx
	push r12
	push r13
	mov rsi, rcx
	sub rdx, 1
	mov rbx, rdx
	xor ecx, ecx
	lfence
	rdpmc
	mov r12, rdx
	mov r13, rax
	call rsi
	xor ecx, ecx
	rdpmc
	lfence
	shl r12, 20h
	or r13, r12
	mov [r9+rbx*8], r13
	shl rdx, 20h
	or rax, rdx
	mov [r8+rbx*8], rax
	pop r13
	pop r12
	pop rbx
	pop rsi
	ret
_lownoiserdpmc ENDP

END
