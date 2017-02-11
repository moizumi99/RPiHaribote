.section ".text"
		
.globl api_getkey 		// void api_getkey(int mode)
api_getkey:
		push	{r0-r12, lr}
		mov		r1, r0
		mov		r0, #15 // get key code
		swi		1
		add		sp, #4		// don't pop r0 which contains the return value
		pop		{r1-r12, pc}
