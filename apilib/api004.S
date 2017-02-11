.section ".text"

.globl	api_end	// void api_end();
api_end:
		mov		r0, #4
		swi		1
		b		.		// this should not happen

