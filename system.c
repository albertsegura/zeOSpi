/*
 * system.c - 
 */

#include <segment.h>
#include <types.h>
#include <interrupt.h>
#include <hardware.h>
#include <system.h>
#include <sched.h>
#include <mm.h>
#include <io.h>
#include <utils.h>
#include <uart.h>
#include <timer.h>
//#include <sem.h> TODO include

int (*usr_main)(void) = (void *) PH_USER_START;

// Pointers to the size of the system and user blocks specified at build/link time
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
	set_worlds_stacks();

	/* Initialize hardware data */
	setIdt(); /* Definicio del vector de interrupcions */

	//__asm__ __volatile__ (
	//	"mov %r7, #4;"
	//	"svc 0x0;"
	//);

	/* Initialize Memory */
	init_mm();

	//test_mmu_funct();

	// Initialize Task queues
	init_freequeue();
	init_readyqueue();
	init_keyboardqueue();
	//init_semarray();

	/* Initialize Raspberry Pi Peripherals */
	init_gpio();
	init_uart();
	init_timer();

	printk("Kernel Loaded!\n");

	/* Initialize an address space to be used for the monoproces version of ZeOS */

	monoprocess_init_addr_space(); /* TO BE DELETED WHEN ADDED THE PROCESS MANAGEMENT CODE TO BECOME MULTIPROCESS */

	//init_sched();

	//init_idle();
	//init_task1();

	/* Move user code/data now (after the page table initialization) */
	copy_data((void *) KERNEL_START + *p_sys_size, usr_main, *p_usr_size);

	printk("Entering user mode...\n");

	//circularbInit(&cbuffer,cbuff, CBUFFER_SIZE);

	enable_int();
/*
	unsigned int old_time = clock_get_time();
	while(1) {
		if (old_time != clock_get_time()) {
			old_time = clock_get_time();
			printint(old_time);
			printc('\n'); printc(13);
		}
	}
*/
	return_gate(USER_ESP, L_USER_START); // TODO canviar el nom de ESP

	/* The execution never arrives to this point */
	return 0;
}
