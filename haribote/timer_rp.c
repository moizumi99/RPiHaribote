#include "bootpack.h"
#include "mmio.h"
#include "mylib.h"

#define timer_handler_max 4
TKernelTimerHandler *timer_handler_list[timer_handler_max];
void *timer_handler_params[timer_handler_max];
void *timer_handler_context[timer_handler_max];
int timer_handler_cnt[timer_handler_max];
int timer_handler_cycle[timer_handler_max];

static unsigned int led_status;
extern char message[];
extern void timer_kbd_handler(unsigned int hTimer, void *pParam, void *pContext);
extern void timer_mouse_handler(unsigned int hTimer, void *pParam, void *pContext);

void init_timer(void)
{
	// Stop timer until the setting is applied
	*TIMER_CONTROL &= 0xffffff00;
	// set timer clock to 1 MHz
	// (0xF9 = 249: timer clock = 250MHz/(249+1))
	*TIMER_PREDIVIDER = 0x000000F9;
	// set timer value
	*TIMER_LOAD   = 10000-1; // 100 Hz
	*TIMER_RELOAD = 10000-1;
	// clear the timer interrupt flag
	*TIMER_IRQ_CLR = 0;
	*TIMER_CONTROL |= 0x000000A2;
}

void timerIRQ_handler(void *pParam)
{
	TKernelTimerHandler *timer_handler;
	unsigned int hTimer;
	static j=0;

	for(hTimer=0; hTimer<timer_handler_max; hTimer++) {
		if (timer_handler_list[hTimer]!=NULL) {
			timer_handler_cnt[hTimer]++;
			if (timer_handler_cnt[hTimer]>timer_handler_cycle[hTimer]) {
				timer_handler_cnt[hTimer] = 0;
				timer_handler = timer_handler_list[hTimer];
				(*timer_handler)(hTimer, timer_handler_params[hTimer], timer_handler_context[hTimer]);
			}
		}
	}

	//	if (j++>50) {
	//		j = 0;
	//		printf("0.5 sec passed\n");
	//	}
	*TIMER_IRQ_CLR = 0;
	inthandler20(NULL);
}	

void print_timer_handler(int hTimer) {
	printf("timer_handler_list[%d]   : 0x%08x\n", hTimer, timer_handler_list[hTimer]);
	printf("timer_handler_params[%d] : 0x%08x\n", hTimer, timer_handler_params[hTimer]);
	printf("timer_handler_context[%d]: 0x%08x\n", hTimer, timer_handler_context[hTimer]);
	printf("timer_handler_cnt[%d]    : 0x%08x\n", hTimer, timer_handler_cnt[hTimer]);
	printf("timer_handler_cycle[%d]  : 0x%08x\n", hTimer, timer_handler_cycle[hTimer]);
}

void timer_led_handler(unsigned int hTimer, void *pParam, void *pContext)
{
	//	volatile unsigned int *led_ptr;
	static unsigned int led_ptr = 0;

	//	led_ptr = (volatile unsigned int *) pParam;
	//	printf("Before: led_ptr: 0x%08x, *led_ptr=%08x\n", led_ptr, *led_ptr);
	if(led_ptr) {
		sprintf(message + strlen(message), "LED: ON\n");
		// digitalWrite(16, LOW);
		//*((volatile unsigned int *) 0x20200028) |= 1 << 16;
	} else {
		sprintf(message + strlen(message), "LED: OFF\n");
		//digitalWrite(16, HIGH);
		//*((volatile unsigned int *) 0x2020001c) |= 1 << 16;
	}
	led_ptr = !led_ptr;
	//	printf("After: led_ptr: 0x%08x, *led_ptr=%08x\n", led_ptr, *led_ptr);
}

void timer_uart_handler(unsigned int hTimer, void *pParam, void *pContext)
{
	unsigned char data;
	struct FIFO32 *kf;
	
	kf = (struct FIFO32 *) pParam;
	if( !(mmio_read(UART0_FR) & (1 <<4)) && !(mmio_read(UART0_FR) & (1<<5))) {
		data = mmio_read(UART0_DR) & 0xFF;
		if (fifo32_put(kf, data) !=0) {
			log_error(4);
		}
	}
	//		/* mmio_write(UART0_ICR, 0x7FF); // Clear all pending interrupts */
}

unsigned StartKernelTimer (unsigned	nHzDelay,	// in HZ units (see "system configuration" above)
			   TKernelTimerHandler *pHandler,
			   void *pParam, void *pContext)	// handed over to the timer handler
{
	int hTimer;
	for(hTimer=0; hTimer<timer_handler_max; hTimer++) {
		if (timer_handler_list[hTimer]==NULL) {
			timer_handler_list[hTimer] = pHandler;
			timer_handler_params[hTimer] = pParam;
			timer_handler_context[hTimer] = pContext;
			timer_handler_cycle[hTimer] = nHzDelay;
			timer_handler_cnt[hTimer] = 0;
			printf("Timer handler %d is added\n", hTimer);
			return hTimer;
		}	
	}
	printf("Error: no timer handler left\n");
	return !0;
}

void CancelKernelTimer (unsigned hTimer)
{
	timer_handler_list[hTimer]=NULL;
}

