.section ".text"

.globl api_refreshwin 		// void api_refresh(int win, int x0, int y0, int x1, int y1);
api_refreshwin:
		push	{r0-r12, lr}
		ldr		r4, [sp, #56]
		ldr		r5, [sp, #60]
		push	{r3, r4, r5}
		mov		r3, r2
		mov		r2, r1
		mov		r1, r0
		mov		r0, #12 // refresh
		swi		1
		pop		{r3, r4, r5}
		pop		{r0-r12, pc}
