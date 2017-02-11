.section ".text"
		
.globl _start
_start:
		push	{r4, lr}
		ldr		r0, =#2		// function 2 = show a string with null stop
		ldr		r1, =#0
		ldr		r2, =msg
		ldr		r3, =#0
		swi		1
		pop		{r4, lr}
_end_app:	
		mov		r0, #4
		swi		1
		b		.			// this address should not realize
msg:
		.string "hello5"
		
