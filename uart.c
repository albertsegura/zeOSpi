/*
 * uart.c -
 */

#include <uart.h>

void uart_toggle_reception(unsigned char enable) {
	unsigned int reg = get_value_from(AUX_MU_CNTL_REG);

	if (enable) reg = reg|0b1;
	else reg = reg&(~0b1);

	set_address_to(AUX_MU_CNTL_REG, reg);
}

void uart_toggle_transmission(unsigned char enable) {
	unsigned int reg = get_value_from(AUX_MU_CNTL_REG);

	if (enable) reg = reg|0b10;
	else reg = reg&(~0b10);

	set_address_to(AUX_MU_CNTL_REG, reg);
}

void init_uart(void) {
	// Set MMU address space
	set_vitual_to_phsycial(AUX_BASE,AUX_BASE_PH,0);

	// Set UART configuration
	set_address_to(AUX_ENABLES, 	0x01); // Enable UART (Allows register modification)


	set_address_to(AUX_MU_CNTL_REG,	0x00);
	set_address_to(AUX_MU_LCR_REG, 	0x03); // Document UART 16550. 8 bits, 1 stop (8N1)
	set_address_to(AUX_MU_MCR_REG,	0x00);
	set_address_to(AUX_MU_IER_REG, 	0x05); // Interruption enabled
	set_address_to(AUX_MU_IIR_REG, 	0xC6); // Enable & Clear FIFO buffers
	set_address_to(AUX_MU_BAUD_REG, BAUDRATE_REG_115200);

	// Set GPIO UART funtion
	uart_toggle_reception(1);
	uart_toggle_transmission(1);
}

Byte uart_interrupt_pend() {
	return ((get_value_from(AUX_MU_IIR_REG)&0b1) == 0);
}

Byte uart_interrupt_pend_rx() {
	return ((get_value_from(AUX_MU_IIR_REG)&0b110) == 0b100);
}

Byte uart_interrupt_pend_tx() {
	return ((get_value_from(AUX_MU_IIR_REG)&0b110) == 0b10);
}

Byte uart_tx_ready() {
	return (get_value_from(AUX_MU_LSR_REG)>>5)&0x1;
}

Byte uart_data_available() {
	return get_value_from(AUX_MU_LSR_REG)&0x1;
}

void uart_send_byte(Byte c) {
	while (!uart_tx_ready());
	set_address_to(AUX_MU_IO_REG,c);
}

Byte uart_get_byte() {
	while (!uart_data_available());
	return get_value_from(AUX_MU_IO_REG);
}
