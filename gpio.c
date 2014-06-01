/*
 * gpio.c -
 */

#include <gpio.h>

void gpio_set_func(unsigned int gpio_pin, Byte func) {
	unsigned int reg_content = 0;
	unsigned int reg_dir = GPFSEL0+(gpio_pin/10)*4;
	unsigned int in_offset = (gpio_pin%10)*3;
	if (gpio_pin > 53) return;

	reg_content = get_value_from(reg_dir);
	reg_content &= ~(0b111<<in_offset);
	reg_content |= (func<<in_offset);
	set_address_to(reg_dir, reg_content);
}

void gpio_set_uart_function() {
	/* Enable GPIO 14&15 Alt. Function 5 (UART1) */
	gpio_set_func(14, GPIO_FUNC_ALT5);
	gpio_set_func(15, GPIO_FUNC_ALT5);

}

void gpio_set_led_function() {
	/* Enable GPIO 16 Output */
	gpio_set_func(16, GPIO_FUNC_OUT);
	gpio_set_led_off();
}

void init_gpio() {
	set_vitual_to_phsycial(GPIO_BASE,GPIO_BASE_PH,0);
	gpio_set_uart_function();
	gpio_set_led_function();
}

void gpio_set_led_on() {
	set_address_to(GPCLR0, 1<<16);
}

void gpio_set_led_off() {
	set_address_to(GPSET0, 1<<16);
}
