.section ".text"

.globl api_fclose 		// void api_fclose(int fhandle);
api_fclose:
		push	{r0-r12, lr}
		mov		r1, r0
		mov		r0, #22
		swi		1
		pop		{r0-r12, pc}
		
