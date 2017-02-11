.section ".text"

.globl api_fopen 		// int api_fopen(char *fname);
api_fopen:
		push	{r0-r12, lr}
		mov		r1, r0
		mov		r0, #21
		swi		1
		add		sp, #4
		pop		{r1-r12, pc}
		
