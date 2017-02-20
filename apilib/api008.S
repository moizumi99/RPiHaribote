.section ".text"

.globl api_initmalloc	// void api_initmalloc()
api_initmalloc:
		push	{r0-r12, lr}
		ldr		r1, =#0x82000000	// start of heap area and memman
		ldr		r2, =#32*1024		// memman area
		add		r2, r1, r2			// start point of free area
		ldr		r3, =#32*1024		// allocated memory (64k - memman size)
		mov		r0, #8				// initmalloc
		swi		1
		pop		{r0-r12, pc}
