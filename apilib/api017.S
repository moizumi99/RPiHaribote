.section ".text"
		
.globl api_inittimer 		// void api_inittimer(int timer, int data);
api_inittimer:
		push	{r0-r12, lr}
		mov		r2, r1
		mov		r1, r0
		mov		r0, #17 // init timer
		swi		1
		pop		{r0-r12, pc}
