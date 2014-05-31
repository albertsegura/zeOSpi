/*
 * system.c - 
 */

#include <hardware.h>
#include <interrupt.h>
#include <io.h>
#include <mm.h>
#include <segment.h>
#include <sched.h>
#include <system.h>
#include <timer.h>
#include <uart.h>
#include <utils.h>

int (*usr_main)(void) = (void *) PH_USER_START;
char uart_read_buff_arr[UART_READ_BUFFER_SIZE];
Circular_Buffer uart_read_buffer;
Sem sem_array[SEM_SIZE];

/* Pointers to the size of the system and user blocks specified at build/link time */
const unsigned int *p_sys_size = (unsigned int *) KERNEL_START+1;
const unsigned int *p_usr_size = (unsigned int *) KERNEL_START+2;

/* This function MUST be 'inline' because it modifies the sp & lr  */
inline void set_initial_stack(void) {
	__asm__ __volatile__ (
		"MOV sp, %0;"
		"MOV lr, #0;"
		:
		: "r"(INITAL_KERNEL_STACK)
	);
}

/* Main entry point to ZEOS Operative System */
int __attribute__((__section__(".text.main"))) main(void) {
	set_initial_stack();
	set_worlds_stacks((unsigned int)INITAL_KERNEL_STACK);

	set_exception_base();

	/* Initialize Memory */
	init_mm();

	//test_mmu_funct();

	/* Initialize Queues&Semaphores */
	init_freequeue();
	init_readyqueue();
	init_keyboardqueue();
	init_semarray();

	/* Initialize Raspberry Pi Peripherals */
	init_gpio();
	gpio_set_led_on(); // TODO test
	init_uart();
	init_timer();


	/*while (1) {
		uart_send_byte('a');
	}*/
	printk("Kernel Loaded!\n");

	//while(1) ;
	//test_mmu_tlb_status();

	init_sched();
	init_idle();
	init_task1();

	circularbInit(&uart_read_buffer,uart_read_buff_arr, UART_READ_BUFFER_SIZE);

	set_interruptions();

	/* Move user code/data now (after the page table initialization) */
	copy_data((void *) KERNEL_START + *p_sys_size, usr_main, *p_usr_size);

	printk("Entering user mode...\n");

	/* Jumps to usr space & enables interrupts */
	return_gate(USER_SP, L_USER_START);

	/* The execution never arrives to this point */
	return 0;
}
