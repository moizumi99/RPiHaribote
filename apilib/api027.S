.section ".text"

.globl api_getlang 		// int api_getlang(void)
api_getlang:
		push	{r0-r12, lr}
		mov		r0, #27
		swi		1
		add		sp, #4
		pop		{r1-r12, pc}
		
