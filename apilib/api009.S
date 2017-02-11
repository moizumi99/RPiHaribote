.section ".text"

.globl api_malloc // void api_malloc(int size)
api_malloc:
		push	{r0-r12, lr}
		mov		r2, r0
		ldr		r1, =#0x80200000	// start of heap area and memmn
		mov		r0, #9
		swi		1
		add		sp, #4		// don't pop r0 which contains the return value
		pop		{r1-r12, pc}
