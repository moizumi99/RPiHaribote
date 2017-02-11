.section ".text"

.globl api_openwin	// int api_openwin(char *buf, int xsiz, int ysiz, int col_inv, char *title);
api_openwin:	
		push 	{r0-r12, lr}
		ldr		r4, [sp, #56]
		push	{r3, r4}
		mov		r3, r2
		mov		r2, r1
		mov		r1, r0
		mov		r0, #5 // window
		swi		1
		pop		{r3, r4}
		add		sp, #4		// don't pop r0 which contains the return value
		pop		{r1-r12, pc}

