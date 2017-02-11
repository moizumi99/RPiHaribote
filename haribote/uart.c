//#include <string.h>

#include "mmio.h"
#include "uart.h"
#include "mylib.h"

void _enable_jtag();

/* Loop <delay> times in a way that the compiler won't optimize away. */
// http://wiki.osdev.org/ARM_RaspberryPi_Tutorial_C
static inline void delay_mmio(int32_t count)
{
	asm volatile("__delay_%=: subs %[count], %[count], #1; bne __delay_%=\n"
		 : "=r"(count): [count]"0"(count) : "cc");
}

// http://wiki.osdev.org/ARM_RaspberryPi_Tutorial_C
void uart_init()
{
	unsigned int reg;
	// Disable AUX (AUX provides mini UART fnction with same ports. Disable before enabling UART)
	mmio_write(AUX_ENABLES, mmio_read(AUX_ENABLES) & (!0x00000001));

	// set pins to the defulat values
	/* mmio_write(GPFSEL0, 0); */
	/* mmio_write(GPFSEL1, 0); */
	/* mmio_write(GPFSEL2, 0); */
	/* mmio_write(GPFSEL3, 0); */
	/* mmio_write(GPFSEL4, 0); */
	/* mmio_write(GPFSEL5, 0); */
	
	// Disable UART0.
	mmio_write(UART0_CR, 0x00000000);
	
	// Setup the GPIO pin 14 && 15 to alto 0
	//    ra=GET32(GPFSEL1);
	reg = mmio_read(GPFSEL1);
    reg &=~(7<<12); //gpio14
    reg |=4<<12;    //alt0
    reg &=~(7<<15); //gpio15
    reg |=4<<15;    //alt0
	mmio_write(GPFSEL1, reg);
	//    PUT32(GPFSEL1,ra);
 
	// Pull up all 
	mmio_write(GPPUD, 0x00000002);
	delay_mmio(150);

	// exclude pin4, 22, 24, 25, 27 used in jtag
	mmio_write(GPPUDCLK0, ~((1 << 4) | (1 << 22) | (1<<24) | (1<<25) | (1<<27)));
	mmio_write(GPPUDCLK1, 0xffffffff);
	delay_mmio(150);
 
	// Write 0 to GPPUDCLK0/1 to make it take effect.
	mmio_write(GPPUDCLK0, 0x00000000);
	mmio_write(GPPUDCLK1, 0x00000000);
	delay_mmio(150);

	// Disable pull up/down for all GPIO pins & delay for 150 cycles.
	mmio_write(GPPUD, 0x00000000);
	delay_mmio(150);
 
	// Disable pull up/down for pin 14,15 & delay for 150 cycles.
	mmio_write(GPPUDCLK0, (1 << 14) | (1 << 15));
	delay_mmio(150);
 
	// Write 0 to GPPUDCLK0 to make it take effect.
	mmio_write(GPPUDCLK0, 0x00000000);
	delay_mmio(150);
 
	// Clear pending interrupts.
	mmio_write(UART0_ICR, 0x7FF);
 
	// Set integer & fractional part of baud rate.
	// Divider = UART_CLOCK/(16 * Baud)
	// Fraction part register = (Fractional part * 64) + 0.5
	// UART_CLOCK = 3000000; Baud = 115200.
 
	// Divider = 3000000 / (16 * 115200) = 1.627 = ~1.
	// Fractional part register = (.627 * 64) + 0.5 = 40.6 = ~40.
	mmio_write(UART0_IBRD, 1);
	mmio_write(UART0_FBRD, 40);
 
	// Enable FIFO & 8 bit data transmissio (1 stop bit, no parity).
	mmio_write(UART0_LCRH, (1 << 4) | (1 << 5) | (1 << 6));
	//	mmio_write(UART0_LCRH, (1 << 5) | (1 << 6));  // fifo disabled
 
	// Mask all interrupts.
	mmio_write(UART0_IMSC, (1 << 1) | (1 << 4) | (1 << 5) | (1 << 6) |
	                       (1 << 7) | (1 << 8) | (1 << 9) | (1 << 10));
 
	// Enable UART0, receive & transfer part of UART.
	mmio_write(UART0_CR, (1 << 0) | (1 << 8) | (1 << 9));

	// enable JTAG
	_enable_jtag();
	
}
 
void uart_putc(unsigned char byte)
{
	// Wait for UART to become ready to transmit.
	while ( mmio_read(UART0_FR) & (1 << 5) ) { }
	mmio_write(UART0_DR, byte);
}
 
unsigned char uart_getc()
{
    // Wait for UART to have recieved something.
    while ( mmio_read(UART0_FR) & (1 << 4) ) { }
    return mmio_read(UART0_DR);
}
 
void uart_write(const unsigned char* buffer, size_t size)
{
	size_t i;
	for (i = 0; i < size; i++ ) {
		//		if (buffer[i]=='n') {
		//			uart_putc('\r');
		//		}
		uart_putc(buffer[i]);
	}
}
 
void uart_puts(const char* str)
{
	uart_write((const unsigned char*) str, strlen(str));
}

