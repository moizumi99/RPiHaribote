#ifndef MMIO_H
#define MMIO_H
#include <stddef.h>
#include <stdint.h>
#include <stdarg.h>

// This part is take from the following tutorial
// http://wiki.osdev.org/ARM_RaspberryPi_Tutorial_C
static inline void mmio_write(uint32_t reg, uint32_t data)
{
	*(volatile uint32_t *)reg = data;
}
 
static inline uint32_t mmio_read(uint32_t reg)
{
	return *(volatile uint32_t *)reg;
}

enum
{
    // The GPIO registers base address.
    GPIO_BASE = 0x20200000,
 
    // The offsets for each register.
	GPFSEL0   = (GPIO_BASE + 0x00),
	GPFSEL1   = (GPIO_BASE + 0x04),
	GPFSEL2   = (GPIO_BASE + 0x08),
	GPFSEL3   = (GPIO_BASE + 0x0c),
	GPFSEL4   = (GPIO_BASE + 0x10),
	GPFSEL5   = (GPIO_BASE + 0x14),
	GPSET0    = (GPIO_BASE + 0x1c),
	GPSET1    = (GPIO_BASE + 0x20),
	GPCLR0    = (GPIO_BASE + 0x28),
	GPLEV0    = (GPIO_BASE + 0x34),
	GPLEV1    = (GPIO_BASE + 0x38),
	GPEDS0    = (GPIO_BASE + 0x40),
	GPEDS1    = (GPIO_BASE + 0x44),
	GPHEN0    = (GPIO_BASE + 0x64),
	GPHEN1    = (GPIO_BASE + 0x68),
 
    // Controls actuation of pull up/down to ALL GPIO pins.
    GPPUD     = (GPIO_BASE + 0x94),
 
    // Controls actuation of pull up/down for specific GPIO pin.
    GPPUDCLK0 = (GPIO_BASE + 0x98),
	GPPUDCLK1 = (GPIO_BASE + 0x9c),
 
    // The base address for UART.
    UART0_BASE = 0x20201000,
 
    // The offsets for reach register for the UART.
    UART0_DR     = (UART0_BASE + 0x00),
    UART0_RSRECR = (UART0_BASE + 0x04),
    UART0_FR     = (UART0_BASE + 0x18),
    UART0_ILPR   = (UART0_BASE + 0x20),
    UART0_IBRD   = (UART0_BASE + 0x24),
    UART0_FBRD   = (UART0_BASE + 0x28),
    UART0_LCRH   = (UART0_BASE + 0x2C),
    UART0_CR     = (UART0_BASE + 0x30),
    UART0_IFLS   = (UART0_BASE + 0x34),
    UART0_IMSC   = (UART0_BASE + 0x38),
    UART0_RIS    = (UART0_BASE + 0x3C),
    UART0_MIS    = (UART0_BASE + 0x40),
    UART0_ICR    = (UART0_BASE + 0x44),
    UART0_DMACR  = (UART0_BASE + 0x48),
    UART0_ITCR   = (UART0_BASE + 0x80),
    UART0_ITIP   = (UART0_BASE + 0x84),
    UART0_ITOP   = (UART0_BASE + 0x88),
    UART0_TDR    = (UART0_BASE + 0x8C),

    // The base address for SYSTEM TIMER
    STIMER_BASE  = 0x20003000,
	STIMER_CLO   = (STIMER_BASE + 0x04),
	STIMER_CHI   = (STIMER_BASE + 0x08),
	
	// AUX
	AUX_BASE     = 0x20215004,
	AUX_ENABLES  = (AUX_BASE) + 0x04
};

#define GPIO_FUNC_INPUT 0b000
#define GPIO_FUNC_OUTPUT 0b001
#define GPIO_FUNC_ALT0 0b100
#define GPIO_FUNC_ALT1 0b101
#define GPIO_FUNC_ALT2 0b110
#define GPIO_FUNC_ALT3 0b111
#define GPIO_FUNC_ALT4 0b010
#define GPIO_FUNC_ALT5 0b011
#define GPIO_NO_PULL 0b00
#define GPIO_PULL_DOWN 0b01
#define GPIO_PULL_UP 0b10

#endif
