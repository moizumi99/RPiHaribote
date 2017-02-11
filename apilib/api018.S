.section ".text"
		
.globl api_settimer 		// void api_settimer(int timer, int time);
api_settimer:
		push	{r0-r12, lr}
		mov		r2, r1
		mov		r1, r0
		mov		r0, #18 // set timer
		swi		1
		pop		{r0-r12, pc}
		
