.section ".text"

.globl api_cmdline 		// int api_cmdline(char *buf, int maxsize);
api_cmdline:
		push	{r0-r12, lr}
		mov		r2, r1
		mov		r1, r0
		mov		r0, #26
		swi		1
		add		sp, #4
		pop		{r1-r12, pc}
		
