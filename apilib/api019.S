.section ".text"
		
.globl api_freetimer 		// void api_freetimer(int timer)
api_freetimer:
		push	{r0-r12, lr}
		mov		r1, r0
		mov		r0, #19 // free timer
		swi		1
		pop		{r0-r12, pc}
		
