.section ".text"

.globl api_putstr1
api_putstr1:
		push	{r0-r12, lr}
		mov		r2, r0	// string
		mov		r3, r1  // length
		mov		r0, #3	// str with length
		mov		r1, #0	// don't care for func 2
		swi 	1
		pop		{r0-r12, pc}

