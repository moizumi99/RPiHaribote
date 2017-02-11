.section ".text.boot"
 
// Make _start global.
.globl _start
 .align
// Entry point for the kernel.
// r15 -> should begin execution at 0x8000.
// r0 -> 0x00000000
// r1 -> 0x00000C42
// r2 -> 0x00000100 - start of ATAGS
// preserve these registers as argument for HariMain
		.equ	und_sp, 0x00310000
		.equ	abr_sp, 0x00320000
		.equ	fiq_sp, 0x00340000
		.equ	irq_sp, 0x00360000
		.equ	svc_sp, 0x00380000
		.equ	sys_sp, 0x00400000
_start:
		// Setup the stack.
		// IRQ mode stack pointer
		// for IRQ, FIQ diable, Thumb disable, IRQ mode (0x12)
		ldr		r0, =0x000000d2
		msr		cpsr, r0
		ldr 	sp, =irq_sp

		// FIQ mode stack pointer
		// for IRQ, FIQ diable, Thumb disable, FIQ mode (0x11)
		ldr		r0, =0x000000d1
		msr		cpsr, r0
		ldr 	sp, =fiq_sp

		// Abort mode stack pointer
		// IRQ and FIQ are disabled, Thumb disabled, Abort mode (0x17)
		ldr		r0, =0x000000d7
		msr		cpsr, r0
		ldr 	sp, =abr_sp

		// Undef mode stack pointer
		// IRQ and FIQ are disabled, Thumb disabled, Undef mode (0x1b)
		ldr		r0, =0x000000db
		msr		cpsr, r0
		ldr 	sp, =und_sp

		//SVC mode stack pointer
		// IRQ, FIQ, disable, Thumb disable, SVC mode (0x13)
		ldr		r0,=0x000000d3
		msr		cpsr, r0
		ldr		sp,=svc_sp

		// IRQ, FIQ, disable, Thumb disable, System mode (0x1F)
		ldr		r0,=0x000000df
		msr		cpsr, r0
		ldr		sp,=sys_sp
		

		
// Enable cache
// This part is taken from the tutorial on the following page
// http://www.valvers.com/open-software/raspberry-pi/step05-bare-metal-programming-in-c-pt5/
		.equ    SCTLR_ENABLE_DATA_CACHE,         0x4
		.equ    SCTLR_ENABLE_BRANCH_PREDICTION,  0x800
		.equ    SCTLR_ENABLE_INSTRUCTION_CACHE,  0x1000

// Enable L1 Cache -------------------------------------------------------
    // R0 = System Control Register
	    mrc p15,0,r0,c1,c0,0

    // Enable caches and branch prediction
  	 	orr r0,#SCTLR_ENABLE_BRANCH_PREDICTION
//    	orr r0,#SCTLR_ENABLE_DATA_CACHE  // Data cache doesn't work until MMU is set. libcsud must be excluded
    	orr r0,#SCTLR_ENABLE_INSTRUCTION_CACHE

    	// System Control Register = R0
	    mcr p15,0,r0,c1,c0,0		

		
// Clear out bss.
		ldr 	r4, _bss_start_p
		ldr 	r9, _bss_end_p
		mov 	r5, #0
		mov 	r6, #0
		mov 	r7, #0
		mov 	r8, #0
		// GNU syntax
		// A numeric local label is a number in the range 0-99.
		// Numeric local labels follow the same syntax as all other labels.
		// Refer to numeric local labels using the following syntax:
		// n{f|b}
		// Where:
		// n is the number of the numeric local label in the range 0-99.
		// f and b instruct the assembler to search forwards and backwards respectively. There is no default. You mu
		//st specify one of f or b.
		b       2f
_bss_start_p:	.word	__bss_start
_bss_end_p:		.word	__bss_end
		
1:	
		// store multiple at r4.
		stmia 	r4!, {r5-r8}
		// If we are still below bss_end, loop.
2:
		cmp 	r4, r9
		blo 	1b
		bl		_enable_jtag
		bl		HariMain
		b		.

		
