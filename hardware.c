/*
 * hardware.c 
 */


#include <types.h>
#include <utils.h>
#include <sched.h>

void return_gate(unsigned int sp, unsigned int pc) {
	__asm__ __volatile__ (
		"cps #0x1F;"
		"mov sp, %0;"
		"mov lr, %1;"
		"cps #0x13;"
		"mrs %0, cpsr;"
		"bic %0, %0, #131;"
		"push {%0};"
		"push {%1};"
		"rfeia sp!"
		: "+r"(sp), "+r"(pc)
	);

}

/* 	STACK offsets
 * +------------+ <- svc		0x1000
 * |			|
 * |			|
 * |			|
 * |			|
 * |			|
 * +------------+ <- fiq, irq	0x500
 * |			|
 * |			|
 * +------------+ <- sys		0x300
 * |			|
 * |			|
 * +------------+ <- abt, und	0x100
 * |			|
 * +------------+ <- 0x0
 */
void set_worlds_stacks(unsigned int stack) {
	__asm__ __volatile__ (
		// Calcule FIQ/IRQ Stack
		"sub %%r0, %0, #0x500;"
		"cps 0x11;" // FIQ
		"mov sp, %%r0;"
		"mov lr, #0;"
		"cps 0x12;" // IRQ
		"mov sp, %%r0;"
		"mov lr, #0;"
		// Calcule SYS Stack
		"sub %%r0, %0, #0x200;"
		"cps 0x1F;" // SYS
		"mov sp, %%r0;"
		"mov lr, #0;"
		// Calcule ABT/UND Stack
		"sub %%r0, %0, #0x200;"
		"cps 0x17;" // ABT
		"mov sp, %%r0;"
		"mov lr, #0;"
		"cps 0x1B;" // UND
		"mov sp, %%r0;"
		"mov lr, #0;"
		// Return to SVC
		"cps 0x13;"
		:
		: "r"(stack)
		: "r0"
	);
}
