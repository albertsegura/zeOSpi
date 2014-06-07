#include <asm.h>

;@ Exceptions Table
ENTRY(exception_vector_table)
	ldr		pc,	[pc, #0x18]
	ldr		pc,	[pc, #0x18]
	ldr		pc,	[pc, #0x18]
	ldr		pc,	[pc, #0x18]
	ldr		pc,	[pc, #0x18]
	ldr		pc,	[pc, #0x18]
	ldr		pc,	[pc, #0x18]
	ldr		pc,	[pc, #0x18]
	.long reset_handler
	.long undefined_instruction_handler
	.long software_interrupt_handler
	.long prefetch_abort_handler
	.long data_abort_handler
	.long 0						// not implemented
	.long interrupt_request_handler
	.long fast_interrupt_request_handler


;@ Exception Routines
.extern reset_routine
.extern undefined_instruction_routine
.extern prefetch_abort_routine
.extern data_abort_routine
.extern interrupt_request_routine
.extern fast_interrupt_request_routine


ENTRY_UA(software_interrupt_routine) ;@ Syscall routine
	push    {lr}
	ldr     r4, =sys_call_table
	ldr		r4, [r4, r7, lsl #2]
	blx		r4
	pop     {pc}

ENTRY_UA(ret_from_fork)
	pop     {r0}	;@ pop the swi routine stacked registers
	mov		r0,	#0
	ldmfd 	sp!, {r4-r12,pc}^


;@ Exception handlers
ENTRY_UA(reset_handler)
	stmfd 	sp!, {r0-r12,lr}
	bl 		undefined_instruction_routine
	ldmfd 	sp!, {r0-r12,pc}^

ENTRY_UA(undefined_instruction_handler)
	stmfd 	sp!, {r0-r12,lr}
	bl 		undefined_instruction_routine
	ldmfd 	sp!, {r0-r12,pc}^

ENTRY_UA(software_interrupt_handler)
	stmfd 	sp!, {r4-r12,lr}
	cps		#0x1F ;@ system
	mov 	r4, sp
	mov		r5, lr
	cps		#0x13 ;@ supervisor
	bic		r6, sp, #0xFF0
	bic		r6, r6, #0xF
	add		r6,	r6,	#0x1C
	stmda 	r6, {r4,r5}
	bl 		software_interrupt_routine
	ldmfd 	sp!, {r4-r12,pc}^

ENTRY_UA(prefetch_abort_handler)
	subs 	lr, lr, #8
	stmfd 	sp!, {r0-r12,lr}
	bl 		prefetch_abort_routine
	ldmfd 	sp!, {r0-r12,pc}^

ENTRY_UA(data_abort_handler)
	subs 	lr, lr, #4
	stmfd 	sp!, {r0-r12,lr}
	bl 		data_abort_routine
	ldmfd 	sp!, {r0-r12,pc}^

ENTRY_UA(interrupt_request_handler)
	srsfd 	sp!, #0x13 ;@ svc
	cpsid	i,	#0x13 ;@ svc
	stmfd 	sp!, {r0-r12,lr}
	cps		#0x1F ;@ system
	mov 	r4, sp
	mov		r5, lr
	cps		#0x13 ;@ svc
	bic		r6, sp, #0xFF0
	bic		r6, r6, #0xF
	add		r6,	r6,	#0x1C
	stmda 	r6, {r4,r5}
	bl 		interrupt_request_routine
	ldmfd 	sp!, {r0-r12,lr}
	pop		{lr}
	sub 	lr, lr, #4
	push 	{lr}
	rfefd	sp!

ENTRY_UA(fast_interrupt_request_handler)
	subs 	lr, lr, #4
	stmfd 	sp!, {r0-r7,lr}
	bl 		fast_interrupt_request_routine
	ldmfd 	sp!, {r0-r7,pc}^

