.section ".text"
		
.globl api_linewin 		// void api_linewin(int win, int x0, int y0, int x1, int y1, int col);
api_linewin:
		push	{r0-r12, lr}
		ldr		r4, [sp, #56]
		ldr		r5, [sp, #60]
		ldr		r6, [sp, #64]
		push	{r3, r4, r5, r6}
		mov		r3, r2
		mov		r2, r1
		mov		r1, r0
		mov		r0, #13 // refresh
		swi		1
		pop		{r3, r4, r5, r6}
		pop		{r0-r12, pc}
