/*
 * hardware.c 
 */


#include <types.h>
#include <utils.h>
#include <sched.h>

void return_gate(unsigned int sp, unsigned int pc) {
	set_world_usr();
	__asm__ __volatile__ (
		"MOV sp, #0;"
		"ORR sp, %0, #0;"
		"MOV lr, #0;"
		"ORR lr, %1, #0;"
		"BX	 lr;"
		:
		: "r"(sp), "r"(pc)
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
 * +------------+ <- sys, mon	0x300
 * |			|
 * |			|
 * +------------+ <- abt, und	0x100
 * |			|
 * +------------+ <- 0x00
 */
void set_worlds_stacks(void) {
	__asm__ __volatile__ (
		// >> Calcule FIQ/IRQ Stack
		"SUB %%r0, %0, #0x500;"
		// >FIQ
		"MRS %%r1, cpsr;"
		"BIC %%r1, %%r1, #0xF;"
		"ORR %%r1, %%r1, #0x1;"
		"MSR cpsr, %%r1;"
		// Set FIQ regs
		"MOV sp, %%r0;"
		"MOV lr, #0;"
		// >IRQ
		"MRS %%r1, cpsr;"
		"BIC %%r1, %%r1, #0xF;"
		"ORR %%r1, %%r1, #0x2;"
		"MSR cpsr, %%r1;"
		// Set IRQ regs
		"MOV sp, %%r0;"
		"MOV lr, #0;"
		// >> Calcule SYS/MON Stack
		"SUB %%r0, %0, #0x200;"
		// >SYS
		"MRS %%r1, cpsr;"
		"ORR %%r1, %%r1, #0xF;"
		"MSR cpsr, %%r1;"
		// Set SYS regs
		"MOV sp, %%r0;"
		"MOV lr, #0;"
		// >MON  TODO El canvi es fa? revisar?
		"MRS %%r1, cpsr;"
		"BIC %%r1, %%r1, #0xF;"
		"ORR %%r1, %%r1, #0x6;"
		"MSR cpsr, %%r1;"
		// Set MON regs
		"MOV sp, %%r0;"
		"MOV lr, #0;"
		// >> Calcule ABT/UND Stack
		"SUB %%r0, %0, #0x200;"
		// >ABT
		"MRS %%r1, cpsr;"
		"BIC %%r1, %%r1, #0xF;"
		"ORR %%r1, %%r1, #0x7;"
		"MSR cpsr, %%r1;"
		// Set ABT regs
		"MOV sp, %%r0;"
		"MOV lr, #0;"
		// >UND
		"MRS %%r1, cpsr;"
		"BIC %%r1, %%r1, #0xF;"
		"ORR %%r1, %%r1, #0xB;"
		"MSR cpsr, %%r1;"
		// Set UND regs
		"MOV sp, %%r0;"
		"MOV lr, #0;"
		// Return to SVC
		"MRS %%r1, cpsr;"
		"BIC %%r1, %%r1, #0xF;"
		"ORR %%r1, %%r1, #0x3;"
		"MSR cpsr, %%r1;"
		:
		: "r"(INITAL_KERNEL_STACK)
		: "r0", "r1"
	);
}
