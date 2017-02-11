.section ".text"

.globl api_fsize 		// int api_fsize(int fhandle, int mode);
api_fsize:
		push	{r0-r12, lr}
		mov		r2, r1
		mov		r1, r0
		mov		r0, #24
		swi		1
		add		sp, #4
		pop		{r1-r12, pc}
		
