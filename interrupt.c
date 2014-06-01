/*
 * interrupt.c -
 */

#include <devices.h>
#include <hardware.h>
#include <interrupt.h>
#include <io.h>
#include <sched.h>
#include <segment.h>
#include <timer.h>
#include <uart.h>

/*char char_map[] =
{
  '\0','\0','1','2','3','4','5','6',
  '7','8','9','0','\'','¡','\0','\0',
  'q','w','e','r','t','y','u','i',
  'o','p','`','+','\0','\0','a','s',
  'd','f','g','h','j','k','l','ñ',
  '\0','º','\0','ç','z','x','c','v',
  'b','n','m',',','.','-','\0','*',
  '\0','\0','\0','\0','\0','\0','\0','\0',
  '\0','\0','\0','\0','\0','\0','\0','7',
  '8','9','-','4','5','6','+','1',
  '2','3','0','\0','\0','\0','<','\0',
  '\0','\0','\0','\0','\0','\0','\0','\0',
  '\0','\0'
};*/


void enable_interrupt_peripheral(unsigned int irq) {
	if (irq < 32) {
		set_address_to(IRQ_ENABLE_1, 1<<irq);
	}
	else if (irq < 64) {
		set_address_to(IRQ_ENABLE_2, 1<<(irq-32));
	}
	else if (irq < 72) {
		set_address_to(IRQ_ENABLE_B, 1<<(irq-64));
	}
}

void disable_interrupt_peripheral(unsigned int irq) {
	if (irq < 32) {
		set_address_to(IRQ_DISABLE_1, 1<<irq);
	}
	else if (irq < 64) {
		set_address_to(IRQ_DISABLE_2, 1<<(irq-32));
	}
	else if (irq < 72) {
		set_address_to(IRQ_DISABLE_B, 1<<(irq-64));
	}
}

/* Exception Routines (except software_interrupt) */
void reset_routine() {
	while(1);
}

void undefined_instruction_routine() {
	while(1);
}

void prefetch_abort_routine() {
	while(1);
}

void data_abort_routine() {
	while(1);
}

void interrupt_request_routine() {
	/*int aux = 0, i;
	__asm__ __volatile__ (
			"mov %0, sp;"
			: "=r"(aux)
	);*/
	/*i = aux-(aux&0xFFF);
	printc('\n'); printc(13);
	printhex(get_value_from(i+0x18));printc(' ');
	printhex(get_value_from(i+0x1c));

	printc('\n'); printc(13);*/

/*	for(i=aux-(aux&0xFFF); i < (aux-(aux&0xFFF)+0x20);i+=4) {
		if ((i&0xF) == 0) {
			printc('\n'); printc(13);
		}
		printhex(get_value_from(i));
		printc('\t');
	}*/
	/*printhex(aux);
	printc('\n'); printc(13);
	for(i=aux; (i&0xFF) != 0;i+=4) {
		if ((i&0xF) == 0) {
			printc('\n'); printc(13);
		}
		printhex(get_value_from(i));
		printc('\t');
	}
	printc('\n'); printc(13);*/

	if (get_value_from(IRQ_PEND_B)&0b1) { // TIMER

		timer_clear_irq();
		clock_increase();
		//printint(clock_get_time());

		//printc('\n'); printc(13);
		sched_update_data();
		if (sched_change_needed()) {
			//printc('i');
			sched_update_queues_state(&readyqueue, current());
			sched_switch_process();
		}
	}
	else if (get_value_from(IRQ_PEND_1)&(1<<29)) { // AUX_UART
		if (uart_interrupt_pend()) {
			if (uart_interrupt_pend_rx()) {
				interrupt_uart_routine();
			}
		}
	}
}

void fast_interrupt_request_routine() {
	while(1);
}

void set_exception_base() {
	unsigned int base = ((unsigned int)&exception_vector_table);
	__asm__ __volatile__ ("MCR P15, 0,  %0,  c12, c0, 0;" :	: "r" (base));
}


void set_interruptions() {
	set_vitual_to_phsycial(IRQ_BASE,IRQ_BASE_PH,0);

	enable_interrupt_peripheral(IRQ_PHPL_AUX);
	enable_interrupt_peripheral(IRQ_PHPL_TIMER);

}

