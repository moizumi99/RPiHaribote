.section ".text"

.globl api_fread 		// int api_fread(int fhandle, int maxsize, int fhandle);
api_fread:
		push	{r0-r12, lr}
		mov		r3, r2
		mov		r2, r1
		mov		r1, r0
		mov		r0, #25
		swi		1
		add		sp, #4
		pop		{r1-r12, pc}
		
