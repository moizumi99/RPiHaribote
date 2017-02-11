.section ".text"

.globl api_fseek 		// void api_fseek(int fhandle, int offset, int mode);
api_fseek:
		push	{r0-r12, lr}
		mov		r3, r2
		mov		r2, r1
		mov		r1, r0
		mov		r0, #23
		swi		1
		pop		{r0-r12, pc}
		
