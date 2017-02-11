.section ".text"

.globl api_putchar // void api_putchar(char r0);
api_putchar:
		push	{r0-r12, lr}
		mov		r1, r0	// r1 = char to show
		ldr		r0, =#1	// func 1 = put 1 char
		ldr		r2, =#0	// don't care for func 1
		ldr		r3, =#0	// don't care for func 1
		swi		1
		pop		{r0-r12, pc}
