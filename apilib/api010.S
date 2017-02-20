.section ".text"

.globl api_free 		// void api_free(char *addr, int size)
api_free:
		push	{r0-r12, lr}
		mov		r3, r1
		mov		r2, r0
		ldr		r1, =#0x82000000	// start of heap area and memman
		mov		r0, #10
		swi 	1
		pop		{r0-r12, lr}
