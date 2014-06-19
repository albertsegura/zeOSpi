#include <devices.h>
#include <hardware.h>
#include <interrupt.h>
#include <io.h>
#include <sched.h>
#include <timer.h>
#include <uart.h>

/* Enable peripheral interrupt */
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

/* Disable peripheral interrupt */
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

/* Exception Routines (except software_interrupt, defined in asm) */
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

/* Set the exception base register */
void set_exception_base() {
	unsigned int base = ((unsigned int)&exception_vector_table);
	__asm__ __volatile__ ("MCR P15, 0,  %0,  c12, c0, 0;" :	: "r" (base));
}


/* Set the interrupts address space & enable peripheral interrupts */
void set_interruptions() {
	set_vitual_to_phsycial(IRQ_BASE,IRQ_BASE_PH,0);

	enable_interrupt_peripheral(IRQ_PHPL_AUX);
	enable_interrupt_peripheral(IRQ_PHPL_TIMER);

}

