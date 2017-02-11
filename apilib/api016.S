.section ".text"
		
.globl api_alloctimer 		// int api_alloctimer(void);
api_alloctimer:
		push	{r0-r12, lr}
		mov		r0, #16 // alloc timer
		swi		1
		add		sp, #4		// don't pop r0 which contains the return value
		pop		{r1-r12, pc}
