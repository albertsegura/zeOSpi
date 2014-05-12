/*
 * interrupt.h - Definició de les diferents rutines de tractament d'exepcions
 */

#ifndef __INTERRUPT_H__
#define __INTERRUPT_H__

#include <types.h>
#include <sched.h>

void setIdt();

#define IRQ_BASE_PH		0x2000B000
#define IRQ_BASE		0xF1000	/* ph 0x2000B000 */

#define IRQ_PEND_B		(IRQ_BASE+0x200)
#define IRQ_PEND_1		(IRQ_BASE+0x204)
#define IRQ_PEND_2		(IRQ_BASE+0x208)

#define IRQ_FIQ_CNTL	(IRQ_BASE+0x20C)

#define IRQ_ENABLE_1	(IRQ_BASE+0x210)
#define IRQ_ENABLE_2	(IRQ_BASE+0x214)
#define IRQ_ENABLE_B	(IRQ_BASE+0x218)

#define IRQ_DISABLE_1	(IRQ_BASE+0x21C)
#define IRQ_DISABLE_2	(IRQ_BASE+0x220)
#define IRQ_DISABLE_B	(IRQ_BASE+0x224)

#define IRQ_PHPL_AUX		29	// AUX_UART
#define IRQ_PHPL_I2S_SPI	43
#define IRQ_PHPL_PWA_0		45
#define IRQ_PHPL_PWA_1		46
#define IRQ_PHPL_SMI		48
#define IRQ_PHPL_GPIO_1		49
#define IRQ_PHPL_GPIO_2		50
#define IRQ_PHPL_GPIO_3		51
#define IRQ_PHPL_GPIO_4		52
#define IRQ_PHPL_I2S		53
#define IRQ_PHPL_SPI		54
#define IRQ_PHPL_PCM		55
#define IRQ_PHPL_UART		57
#define IRQ_PHPL_TIMER		64
#define IRQ_PHPL_MAILBOX	65
#define IRQ_PHPL_DOORBELL_0	66
#define IRQ_PHPL_DOORBELL_1	67
#define IRQ_PHPL_GPU_0		68
#define IRQ_PHPL_GPU_1		69
#define IRQ_PHPL_ILLEGAL_0	70
#define IRQ_PHPL_ILLEGAL_1	71

extern unsigned int exception_vector_table;

/* EXCEPTION VECTOR (Offsets from base)
 * Reset 					0x00000000
 * Undefined instruction 	0x00000004
 * Software interrupt 		0x00000008
 * Prefetch abort 			0x0000000c
 * Data abort 				0x00000010
 * Reserved 				0x00000014
 * Interrupt request 		0x00000018
 * Fast interrupt request 	0x0000001c
 */


void reset_routine();
void undefined_instruction_routine();
void prefetch_abort_routine();
void data_abort_routine();
void interrupt_request_routine();
void fast_interrupt_request_routine();

void enable_int(void);


#endif  /* __INTERRUPT_H__ */
