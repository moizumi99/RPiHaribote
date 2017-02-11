.section .text

// from svc mode to svc mode
.globl _cur_pcb
_cur_pcb:	.word	0
.globl _next_pcb
_next_pcb:	.word	0
		
_IRQ_interrupt:
		// IRQ uses common sp (sp_irq) for all processes. Interrupt must be disabled throughout the process
		// save the registers to sp_irq stack. r13! means r13 will be +14x4 after this
		stmdb	sp!, {r0-r12, lr} // dtmdb = stmfd
		// call the interrupt hander in C language
		ldr		r0, =#0
		bl		IRQ_handler
		// retrieve the registers from stack
		cmp		r0, #0	// r0 holds next TSS address. Null means no task switch
		ldmeqia	sp!, {r0-r12, lr}  // ldmia = ldmfd, eq = if equal
		// retrieve return address (lr - 4) this should switch mode as well
		subeqs	pc, lr, #4
// task switch from IRQ to usr or system mode
_TaskSwitchIRQ:
//		b	. // TEST
		str		r0, _next_pcb  		// save next pcb adr
		ldmia	sp!, {r0-r12, lr} 	// restore all registers at IRQ entry from sp_irq
		str		sp, _sp_save		// Save SP_irq
		ldr		sp, _cur_pcb		// load current pcb adr, sp = pcb->cpsr
		add		sp, #8				// move to PCB.r0 position
		stmia	sp, {r0-lr}^		// store all usr registers (sp_usr, lr_usr)
		mrs		r0, spsr			// 
		stmdb	sp, {r0, lr}		// spsr and return address
//		restore next pcb and return
//		b	. // TEST
		ldr		r0, _next_pcb		// r0 = &pcb_b
		str		r0, _cur_pcb
		// switch virtual memory table
		ldr		r0, [r0, #0x54]  	// load vmem table address
		bl		switch_vmem
		//  retrieve next registers
//		b	. // TEST
		ldr		r0, _next_pcb		// r0 = &pcb_b
		add		sp, r0, #8				// move sp to PCB.r0 position
		ldmdb	sp, {r0, lr}
		msr		spsr, r0
		ldmia	sp, {r0-lr}^
		ldr		sp, _sp_save  		// retrieve sp_irq
		nop
		subs	pc, lr, #4
		// code should not reach here
		b		.
_sp_save:	.word	0
		
		// switch to the task specified by r0
.globl	_task_switch_asm
_task_switch_asm:
		cpsid	i					// disable interrupt
		str		r0, _r0_save
		str		lr, _lr_save
		mrs		r0, cpsr
		cps		#0x12				// IRQ mode
		msr		spsr, r0
		ldr		r0, _r0_save
		ldr		lr, _lr_save
		add		lr, lr, #4			// pretend that program reached here with IRQ
		stmdb	sp!, {r0-r12, lr} 	// stmdb = stmfd
		b		_TaskSwitchIRQ
_r0_save:	.word	0
_lr_save:	.word	0
		
// Supervisor call
_SWI_interrupt:
//		b	.	// TEST
		cmp		r0, #4
		beq		_app_end		// if r0=4, app end
		// use its process's svc process
		str		r0, _r0_swi_save
		ldr		r0, _cur_pcb
		ldr		sp, [r0, #0x44]	// sp_svc
		push	{lr}
		mrs		lr, cpsr
		push	{lr}
		str		sp, [r0, #0x44]
		ldr		r0, _r0_swi_save
		// return to system mode
		cpsie	i, #0x1f
		bl		hrb_api
		// svc mode
		cpsid	i, #0x13
//		b	.	// TEST
		str		r0, _r0_swi_save  // save the return value
		ldr		r0, _cur_pcb
		ldr		sp, [r0, #0x44]	// sp_svc
		pop		{lr}
		msr		cpsr, lr
		pop		{lr}
		str		sp, [r0, #0x44]
		ldr		r0, _r0_swi_save
		movs	pc, lr
_app_end:
		cps		#0x1f
		ldr		r0, _cur_pcb
		ldr		r1, =#0
		str		r1, [r0, #0x50] // make run_flag=0
		ldr		sp, [r0, #0x48]	// load sp_sys
		pop		{r0}			// pop cpsr that was pushed by start_app
		msr		cpsr, r0		// restore to spsr
		pop		{r0-r12, pc}	// move back to the caller of start_app and change mode
_r0_swi_save:	 .word	0

		
// _start_app() starts application.
.global	start_app
start_app:	//void start_app(int pc, int sp) // sp = stack pointer
		// r0 = next pc
		// r1 = next sp
		push	{r0-r12, lr}		// this will be popped by _app_end
		bl		invalidate_tlbs		// invalidate tlb and flush cache
		mrs		r2, cpsr
		push	{r2}				// push cpsr
		ldr		r3, _cur_pcb		// r3 points to current pcb data set
		str		sp, [r3, #0x48]		// save sp_sys
		ldr		r4, =#01			// 
		str		r4, [r3, #0x50] 	// set run_flag=1
		bic		r2, #0x1F			// clear mode select
		orr		r2, #0x10			// set usr mode (0x10)
//		orr		r2, #0x1f			// set system mode (0x1f) for debugging (swi emulation)
		mov		sp, r1				// app stack pointer = r1
		str		sp, [r3, #0x4c]		// save to sp_usr
		ldr		lr, =_app_return	// return location from the app
		cpsid	i					// disable IRQ
		cps		#0x13				// svc mode
		ldr		sp, [r3, #0x44]		// load sp_svc
		push	{r0}				// next PC address
		msr		spsr_cxsf, r2		// spsr = system cpsr
		add		sp, #4				// adjust for the last pop
		str		sp, [r3, #0x44]		// store sp_svc
		ldmdb	sp, {pc}^			// pop new pc, and change mode (cpsr <= spsr)
_app_return:						// usually the following is not used. Only used when app is run in system mode for debug, and retured instead of swi
		cpsid	i, #0x1F			// system mode
		b		_app_end
		
// enable MMU function void enableMMU(uint32_t table_base, uint32_t flag)
.globl start_MMU
start_MMU:
	    mov r2,#0
	    mcr p15,0,r2,c7,c7,0 ;@ invalidate caches
	    mcr p15,0,r2,c8,c7,0 ;@ invalidate tlb
	    mcr p15,0,r2,c7,c10,4 ;@ DSB ??

    	mvn r2,#0
	    bic r2,#0xe			// domain0: client, domain1: no access
	    mcr p15,0,r2,c3,c0,0 ;@ domain

    	mcr p15,0,r0,c2,c0,0 ;@ tlb base
	    mcr p15,0,r0,c2,c0,1 ;@ tlb base

	    mrc p15,0,r2,c1,c0,0
	    orr r2,r2,r1
//		orr	r2, r2, #1
	    mcr p15,0,r2,c1,c0,0
    	bx lr		

.globl invalidate_tlbs
invalidate_tlbs:
	    mov 	r2,#0
	    mcr 	p15,0,r2,c7,c7,0 ;@ invalidate caches
	    mcr 	p15,0,r2,c8,c7,0  ;@ invalidate tlb
	    mcr 	p15,0,r2,c7,c10,4 ;@ DSB ??
	    bx 		lr
		
.globl switch_vmem // r0: next vmem table, r4: scratch register
switch_vmem:
//		// disable MMU
//	    mrc 	p15,0,r1,c1,c0,0
//		bic		r1, #1
//	    mcr 	p15,0,r1,c1,c0,0

		// invalidate cache and tlb
		mov		r1, #0
	    mcr 	p15,0,r1,c7,c7,0 ;@ invalidate caches
	    mcr 	p15,0,r1,c8,c7,0  ;@ invalidate tlb
	    mcr 	p15,0,r1,c7,c10,4 ;@ DSB ??

		// set new tlb
	   	mcr 	p15,0,r0,c2,c0,0 ;@ tlb base
	    mcr 	p15,0,r0,c2,c0,1 ;@ tlb base


		// enable MMU again
//	    mrc 	p15,0,r1,c1,c0,0
//	    orr 	r1, r1, #1
//	    mcr 	p15,0,r1,c1,c0,0
//	    bx 		lr
		
// Enable JTAG debugging ---------------------------------------
.globl _enable_jtag
_enable_jtag:	
		ldr 	r0, _gpio_address
		// set pin4 to ALT5
		mov		r3, #0b010
		lsl		r1, r3, #12
		str 	r1, [r0]
		// set pin22 to ALT4
		mov		r2, #0b011
		lsl 	r1, r2, #6
		// set pin24 to ALT4
		orr 	r1, r1, r2, lsl #12
		// set pin25 to ALT4
		orr 	r1, r1, r2, lsl #15
		// set pin27 to ALT4
		orr 	r1, r1, r2, lsl #21
		str 	r1, [r0, #8]
		bx		lr
// GPIO address
.globl _gpio_address
_gpio_address:	.word	0x20200000
.globl _gpio_address_end
_gpio_address_end:
		mov		r0,r0	// this meaningless sentence inserted to preserve the label
		

// Data abort
.globl _DataAbort_interrupt
_DataAbort_interrupt:
		// todo: when true paging is added, handling of pages must be added here
		push	{r0-r12, lr}
		cps		#0x1f
		mov		r0, sp
		mov		r1, lr
		cps		#0x17
		push	{r0, r1}
		mov		r0, lr
		mov		r1, sp
		mrs		r2, spsr
		cps		#0x1f
		bl		inthandler0d
		cps		#0x17
		cmp		r0, #0
		bne		_dab_err
		pop		{r0, r1}
		pop		{r0-r12, lr}
		subs	pc, lr, #-4
		// Error handling
_dab_err:	
		pop		{r0, r1}
		pop		{r0-r12, lr}
		cps 	#0x1f
		mov		r0,#4
		swi 	1
		b		.

// Prefetch abort
.globl _PrefAbort_interrupt
_PrefAbort_interrupt:
		// to do when true paging is added, handling of pages must be added here
		push	{r0-r12, lr}
		cps		#0x1f
		mov		r0, sp
		mov		r1, lr
		cps		#0x17
		push	{r0, r1}
		mov		r0, lr
		mov		r1, sp
		mrs		r2, spsr
		cps		#0x1f
		bl		inthandler0e
		cps		#0x17
		cmp		r0, #0
		bne		_pab_err
		pop		{r0, r1}
		pop		{r0-r12, lr}
		subs	pc, lr, #-4
		// Error handling
_pab_err:	
		pop		{r0, r1}
		pop		{r0-r12, lr}
		cps 	#0x1f
		mov		r0,#4
		swi 	1

// Undefined instruction
.globl _Undef_interrupt
_Undef_interrupt:
		// to do when true paging is added, handling of pages must be added here
		push	{r0-r12, lr}
		mov		r0, lr
		cps		#0x13			// move to SVC mode
		cpsie	i				// enable IRQ
		bl		inthandler0f
		cmp		r0, #0
		cpsid	i
		bne		_udf_err
		cps		#0x1b
		pop		{r0-r12, lr}
		movs	pc, lr
		// Error handling
_udf_err:
		cps		#0x1b
		pop		{r0-r12, lr}
		cps 	#0x1f
		mov		r0,#4
		swi 	1
		b		.
		
// FIQ interrupt
.globl _FIQ_interrupt
_FIQ_interrupt:
		subs	pc, lr, #4
		

// Useful functions
		
// Jump to arbitrary address r0
.globl farjmp
farjmp:
		bx		r0

.globl io_hlt
io_hlt:	
halt:
		wfe
		bx		lr


.globl	_hangup
_hangup:
		// IRQ, FIQ disable, Thumb disable, SVC mode
		ldr		r0,=0x000000d3
		msr		cpsr, r0
		b		.

.globl io_stihlt
io_stihlt:
		cpsie	i
		wfe
		// bx: branch and exchange instruction set (if bit[0]=0, ARM state, if 1, Thumb)
		bx		lr
		
// return CPSR value
.globl io_load_eflags
io_load_eflags:	
		mrs		r0, cpsr
		bx 		lr

// putback CPSR value
.globl io_store_eflags
io_store_eflags:
		msr 	cpsr_c, r0
		bx		lr

// flush cache
.globl flush_cache_full
flush_cache_full:	
		mcr 	p15, #0, r0, c7, c14, #0
		bx 		lr
		

.globl asm_end_app
asm_end_app:
		mov		r0, #4
		swi		1
		b		.		// this should not happen

.globl PUT32		// void PUT32(unsigned int, unsigned int);
PUT32:
		str		r1,[r0]
		bx lr

.globl	GET32		// unsigned int GET32(unsigned int)
GET32:
		ldr		r0, [r0]
		bx lr

.globl dummy		// void dummy(unsigned int)
dummy:	
		bx lr

		
.globl _initial_vector_start
_initial_vector_start:
		ldr		pc, _vec_Reset
		ldr		pc, _vec_Undef
		ldr		pc, _vec_SWI
		ldr		pc, _vec_PrefAbort
		ldr		pc, _vec_DataAbort
		ldr		pc, _vec_Reserved
		ldr		pc, _vec_IRQ
		ldr		pc, _vec_FIQ
_vec_Reset:			.word	_start
_vec_Undef:			.word	_Undef_interrupt
_vec_SWI:			.word	_SWI_interrupt
_vec_PrefAbort:		.word	_PrefAbort_interrupt
_vec_DataAbort:		.word	_DataAbort_interrupt
_vec_Reserved:		.word	_hangup
_vec_IRQ:			.word	_IRQ_interrupt
_vec_FIQ:			.word	_FIQ_interrupt
.globl	_initial_vector_end
_initial_vector_end:
		mov		r0,r0	// this meaningless sentence inserted to preserve the label _initial_vector_end

