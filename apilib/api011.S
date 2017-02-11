.section ".text"

.globl api_point 		// void api_point(int win, int x, int y, int col);
api_point:
		push	{r0-r12, lr}
		ldr		r4, [sp, #56]
		push	{r3, r4}
		mov		r3, r2
		mov		r2, r1
		mov		r1, r0
		mov		r0, #11 // dot
		swi		1
		pop		{r3, r4}
		pop		{r0-r12, pc}
