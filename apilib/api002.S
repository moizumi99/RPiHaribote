.section ".text"

.globl api_putstr0
api_putstr0:
		push	{r0-r12, lr}
		mov		r2, r0
		mov		r0, #2	// str with null end
		mov		r1, #0	// don't care for func 2
		mov		r3, #0  // don't care for func 2
		swi 	1
		pop		{r0-r12, pc}

