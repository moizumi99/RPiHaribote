.section ".text"

.globl api_boxfilwin	// int api_rwin(int win, int x, int y, int col, int len, char *str);
api_boxfilwin:	
		push 	{r0-r12, lr}
		ldr		r4, [sp, #56]
		ldr		r5, [sp, #60]
		push	{r3, r4, r5}
		mov		r3, r2
		mov		r2, r1
		mov		r1, r0
		mov		r0, #7 // box fil
		swi		1
		pop		{r3, r4, r5}
		add		sp, #4		// don't pop r0 which contains the return value
		pop		{r1-r12, pc}
