#include "bootpack.h"
#include "mylib.h"

#define IRQ_NUM 71

int IRQ_handler(void);

TInterruptHandler *IRQ_handler_list[IRQ_NUM];
void *IRQ_handler_params[IRQ_NUM];

unsigned int *monitor_ptr;
register char * stack_ptr asm ("sp");
uint32_t next_pcb;

void set_vector_table(void);

extern char message[];

void init_pic(void)
{
	next_pcb = 0;
	
	// set interrupt vector table
	set_vector_table();
	
	// disable all interrupts
	*INTERRUPT_DISABLE_BASIC_IRQS = 0xffffffff;
	*INTERRUPT_DISABLE_IRQS1 = 0xffffffff;
	*INTERRUPT_DISABLE_IRQS2 = 0xffffffff;
	*INTERRUPT_FIQ_CTRL = 0;

	// Add Timer IRQ handler
	ConnectInterrupt(64, timerIRQ_handler, NULL);
}

void set_vector_table(void)
{
	extern void *_initial_vector_start;
	extern void *_initial_vector_end;
	// put volatile to avoid optimization
	volatile unsigned int *vec;
	volatile unsigned int *p;
	volatile unsigned int *s = (unsigned int *) &_initial_vector_start;
	volatile unsigned int *e = (unsigned int *) &_initial_vector_end;

	printf("Vector table check\n");
	printf("_initial_vector_start: %08x\n", (unsigned int) s);
	printf("_initial_vector_end  : %08x\n", (unsigned int) e);

	vec = 0;
	//	printf("Addr : Hex\n");
	for(p = s; p<e; p++) {
		*vec = *p;
		//		printf("0x%02x : 0%08x\n", (unsigned int) vec-1, *(vec-1));
		vec++;
	}
	return;
}	


int IRQ_handler(void) {
	//	unsigned char data;
	TInterruptHandler *IRQ_handler;
	int i;
	unsigned int irq_condition;
	uint32_t result;
	
	io_cli();
	
	//for(i=0; i<IRQ_NUM; i++) {
	for(i=64; i<65; i++) { // currently only timer IRQ (i=64) is used
		if (i<32) {
			irq_condition = (*INTERRUPT_IRQ_PENDING1 & (1 << i));
		} else if (i<64) {
			irq_condition = (*INTERRUPT_IRQ_PENDING2 & (1 << (i-32)));
		} else {
			irq_condition = (*INTERRUPT_IRQ_BASIC_PENDING & (1 << (i-64)));
		}
		if (irq_condition && IRQ_handler_list[i]!=NULL) {
			IRQ_handler = IRQ_handler_list[i];
			(*IRQ_handler)(IRQ_handler_params[i]);
		}
	}
	if (next_pcb==0) {
		result = 0;
	} else {
		//		printf("next_pcb: %08x\n", next_pcb);
		result = next_pcb;
		next_pcb = 0;
	}
	return result;
}

void interrupt_test(void)
{
	/* uint32_t reg; */
	unsigned timer_led_handle, timer_uart_handle, timer_kbd_handle, timer_mouse_handle;

	//add UART timer handler
	//	timer_uart_handle = StartKernelTimer(1, timer_uart_handler, NULL, NULL);
	//	printf("Timer UART: %d\n", timer_uart_handle);

	// add USB timer keyboard handler
	timer_kbd_handle = StartKernelTimer(2, timer_kbd_handler, NULL, NULL);
	printf("Timer KBD: %d\n", timer_kbd_handle);

	// add USB mouse timer handler
	timer_mouse_handle = StartKernelTimer(2, timer_mouse_handler, NULL, NULL);
	printf("Timer Mouse: %d\n", timer_mouse_handle);
}

void ConnectInterrupt(unsigned nIRQ, TInterruptHandler *pHandler, void *pParam)
{
	unsigned int reg;
	if (nIRQ!=64 && nIRQ!=57 && nIRQ!=9) {
		printf("Error: only IRQ=65 (timer), 57 (uart), and 9 (USB?) are supported yet.\n");
		while(1) {};
		return;
	}
	if (nIRQ<32) {
		reg = *INTERRUPT_ENABLE_IRQS1;
		reg = reg | (1<<nIRQ);
		*INTERRUPT_ENABLE_IRQS1 = reg;
	} else if (nIRQ<64) {
		reg = *INTERRUPT_ENABLE_IRQS2;
		reg = reg | (1<< (nIRQ-32));
		*INTERRUPT_ENABLE_IRQS2 = reg;
	} else if (nIRQ<IRQ_NUM) {
		reg = *INTERRUPT_ENABLE_BASIC_IRQS;
		reg = reg | (1<< (nIRQ-64));
		*INTERRUPT_ENABLE_BASIC_IRQS = reg;
	}
	if(nIRQ<IRQ_NUM) {
		IRQ_handler_list[nIRQ] = pHandler;
		IRQ_handler_params[nIRQ] = pParam;
	}
}

unsigned int hold_IRQ()
{
	unsigned int cpsr;
	cpsr = io_load_eflags();
	if (!(cpsr & 0x00000080)) {
		io_cli();
		return 1;
	} else {
		return 0;
	}
}

void unhold_IRQ(unsigned int irq_status)
{
	if (irq_status) {
		io_sti();
	}
}

