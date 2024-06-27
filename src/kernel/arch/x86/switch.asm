global do_switch

; Context switching typically involves saving the state of the currently running process before switching to another.
; The stack is a common data structure used to store temporary data and function call information.

do_switch:
	; Retrieves the address of the current process's state from the stack and loads the address of the current process context into esi
	mov esi, [esp]
	pop eax

	; save current process onto global stack
	push dword [esi+4]	; eax
	push dword [esi+8]	; ecx
	push dword [esi+12]	; edx
	push dword [esi+16]	; ebx
	push dword [esi+24]	; ebp
	push dword [esi+28]	; esi
	push dword [esi+32]	; edi
	push dword [esi+48]	; ds
	push dword [esi+50]	; es
	push dword [esi+52]	; fs
	push dword [esi+54]	; gs

	; Remove the mask from the (Programmable Interrupt Controller) PIC
	; 0x20 = end-of-interrupt (EOI) signal
	mov al, 0x20
	out 0x20, al

	; Load the page table
	mov eax, [esi+56]
	mov cr3, eax

	; restore the registers into the CPU so that it can immediately start working on the new context
	pop gs
	pop fs
	pop es
	pop ds
	pop edi
	pop esi
	pop ebp
	pop ebx
	pop edx
	pop ecx
	pop eax

	; When we pop these values, the corresponding CPU registers will be loaded with the retrieved values. The specific ordering of the pop instructions ensures that each retrieved value (copy of a saved register) is loaded into the appropriate CPU register

	; return
	iret
