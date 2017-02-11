.section ".text"
		
.globl HariMain
HariMain:	
		push	{r4, lr}
		ldr		r0, =#2		// function 2 = show a string with null stop
		ldr		r1, =#0
		ldr		r2, =msg
		ldr		r3, =#0
		swi		1
_end_app:	
		mov		r0, #4
		swi		1
		b		.
		
msg:
		.string "hello2"
		
