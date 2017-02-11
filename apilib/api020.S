.section ".text"

.globl api_beep 		// void api_beep(int tone);
api_beep:
		push	{r0-r12, lr}
		mov		r1, r0
		mov		r0, #20 // free timer
		swi		1
		pop		{r0-r12, pc}
		
