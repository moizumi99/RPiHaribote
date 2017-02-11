.section ".text"
		
.globl api_closewin 		// void api_closewin(int win)
api_closewin:
		push	{r0-r12, lr}
		mov		r1, r0
		mov		r0, #14 // sheet free
		swi		1
		pop		{r0-r12, pc}
		
