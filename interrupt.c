/*
 * interrupt.c -
 */
#include <types.h>
#include <interrupt.h>
#include <segment.h>
#include <hardware.h>
#include <io.h>
#include <uart.h>
#include <timer.h>


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
	if (get_value_from(IRQ_PEND_B)&0b1) { // TIMER
		timer_clear_irq();
		clock_increase();

		sched_update_data();
		if (sched_change_needed()) {
			sched_update_queues_state(&readyqueue, current());
			sched_switch_process();
		}
	}
	else if (get_value_from(IRQ_PEND_1)&(1<<29)) { // AUX_UART
		Byte data;
		if (uart_interrupt_pend()) {
			if (uart_interrupt_pend_rx()) {
				data = uart_get_byte();
				// TODO change to fill buffer
				printc(data);
				printc('\n'); printc(13);
			}
		}
	}
}

void fast_interrupt_request_routine() {
	while(1);
}

void setIdt() {
 	/* EXCEPTIONS */
	unsigned int base = ((unsigned int)&exception_vector_table);
	__asm__ __volatile__ ("MCR P15, 0,  %0,  c12, c0, 0;" :	: "r" (base));

	/* INTERRUPTIONS */
	// Handlers set at compiling time.
}



void enable_int(void) {
	set_vitual_to_phsycial(IRQ_BASE,IRQ_BASE_PH,0);

	enable_interrupt_peripheral(IRQ_PHPL_AUX);	// AUX_UART
	enable_interrupt_peripheral(IRQ_PHPL_TIMER);

	// TODO enable for more worlds ?
	cpsr reg;
	reg.entry = read_cpsr();
	reg.bits.dI = 0;
	write_cpsr(reg);
}

