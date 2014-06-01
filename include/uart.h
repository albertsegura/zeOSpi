/*
 * uart.h
 */

#ifndef __UART_H__
#define __UART_H__

#include <types.h>
#include <utils.h>
#include <mm.h>
#include <gpio.h>


/* Coded following the Broadcom BCM2835 ARM Peripherals and TI PC16550D datasheets */
#define AUX_BASE_PH		0x20215000
#define AUX_BASE		0xF0000	/* ph 0x20215000 */
#define AUX_IRQ			(AUX_BASE+0x00)
#define AUX_ENABLES		(AUX_BASE+0x04)
#define AUX_MU_IO_REG	(AUX_BASE+0x40)
#define AUX_MU_IER_REG	(AUX_BASE+0x44)
#define AUX_MU_IIR_REG	(AUX_BASE+0x48)
#define AUX_MU_LCR_REG	(AUX_BASE+0x4C)
#define AUX_MU_MCR_REG	(AUX_BASE+0x50)
#define AUX_MU_LSR_REG	(AUX_BASE+0x54)
#define AUX_MU_MSR_REG	(AUX_BASE+0x58)
#define AUX_MU_SCRATCH	(AUX_BASE+0x5C)
#define AUX_MU_CNTL_REG	(AUX_BASE+0x60)
#define AUX_MU_STAT_REG	(AUX_BASE+0x64)
#define AUX_MU_BAUD_REG	(AUX_BASE+0x68)

/* Calculated using formula on Broadcom datasheet:
 * baudrate_reg = ((system_clock_freq/(baudrate*8))-1);
 * where system_clock_freq == 250MHz.	*/
#define BAUDRATE_REG_115200	270
#define BAUDRATE_REG_9600	3254


void init_uart();

Byte uart_interrupt_pend();
Byte uart_interrupt_pend_rx();
Byte uart_interrupt_pend_tx();

void uart_send_byte(Byte c);
Byte uart_get_byte();

#endif  /* __UART_H__ */
