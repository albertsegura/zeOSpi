#include <sched.h>
#include <mm.h>
#include <io.h>
#include <sem.h>
#include <hardware.h>
#include <system.h>

union task_union task[NR_TASKS] __attribute__((__section__(".data.task")));
struct task_struct * idle_task;

struct list_head freequeue;
struct list_head readyqueue;
struct list_head keyboardqueue;

int lastPID;
unsigned int rr_quantum;

/* get_DIR - Returns the Page Directory address for task 't' */
fl_page_table_entry * get_DIR (struct task_struct *t) {
	return t->dir_pages_baseAddr;
}

/* get_PT - Returns the Page Table address for task 't' */
sl_page_table_entry * get_PT (struct task_struct *t, unsigned char dir_entry) {
	return (sl_page_table_entry *)(((unsigned int)(t->dir_pages_baseAddr[dir_entry].bits.pbase_addr))<<10);
}

/* Idle task function */
void cpu_idle() {
	asm volatile("cpsie i, #0x13;"); // enable irq on SYS
	while(1);
}

/* Get task_struct of the process from the queue with the especified PID  */
int getStructPID(int PID, struct list_head * queue, struct task_struct ** pointer_to_desired) {
	int found = 0;

	if (current()->PID == PID) {
		*pointer_to_desired = current();
		found = 1;
	}

	if (!list_empty(queue) && (found == 0)) {
		struct list_head *first = list_first(queue);
		if (list_head_to_task_struct(first)->PID == PID) {
			found = 1;
			*pointer_to_desired = list_head_to_task_struct(first);
		}
		else {
			list_del(queue);
			list_add_tail(first, queue);
		}

		while(first != list_first(queue)) {
			struct list_head *act = list_first(queue);
			if (list_head_to_task_struct(act)->PID == PID) {
				found = 1;
				*pointer_to_desired = list_head_to_task_struct(act);
			}
			list_del(queue);
			list_add_tail(act, queue);
		}
	}

	return found;
}

/* Init freequeue */
void init_freequeue () {
	int i;

	INIT_LIST_HEAD(&freequeue);
	for (i = 0; i < NR_TASKS; ++i) {
		list_add_tail(&task[i].task.list,&freequeue);
	}
}

/* Init readyqueue */
void init_readyqueue () {
	INIT_LIST_HEAD(&readyqueue);
}

/* Init keyboardqueue */
void init_keyboardqueue () {
	INIT_LIST_HEAD(&keyboardqueue);
}

/* Init Semaphores */
void init_semarray() {
	int i;

	for(i=0;i<SEM_SIZE;++i)	{
		sem_array[i].id = i;
		sem_array[i].pid_owner = -1;
		sem_array[i].value = 0;
	}
}

/* Idle task initialization */
void init_idle () {	
	struct list_head *idle_list_pointer = list_first(&freequeue);
	list_del(idle_list_pointer);
	idle_task = list_head_to_task_struct(idle_list_pointer);
	union task_union *idle_union_stack = (union task_union*)idle_task;
	allocate_page_dir(idle_task);

	idle_task->PID = 0;
	idle_union_stack->task.kernel_sp = (unsigned long)&idle_union_stack->stack[KERNEL_STACK_SIZE-1];
	idle_union_stack->task.kernel_lr = (unsigned long)&cpu_idle;

	/* Stats initialization */
	idle_task->statistics.cs = 0;
	idle_task->statistics.tics = 0;
	idle_task->statistics.remaining_quantum = 0;
	idle_task->process_state = ST_READY;
}

/* Task1 initialization */
void init_task1() {
	struct list_head *task1_list_pointer = list_first(&freequeue);
	list_del(task1_list_pointer);
	struct task_struct * task1_task_struct = list_head_to_task_struct(task1_list_pointer);
	allocate_page_dir(task1_task_struct);
	fl_page_table_entry * dir_task1 = get_DIR(task1_task_struct);

	task1_task_struct->PID = 1;
	lastPID = 1;
	set_user_pages(task1_task_struct);
	mmu_change_dir(dir_task1);

	get_newpb(task1_task_struct);

	/* Stats initialization */
	task1_task_struct->statistics.cs = 0;
	task1_task_struct->statistics.tics = 0;
	task1_task_struct->statistics.remaining_quantum = DEFAULT_RR_QUANTUM;
	task1_task_struct->process_state = ST_RUN;
}

/* Init scheduler */
void init_sched() {
	init_Sched_RR();
}

/* Task switch wrapper */
void task_switch_wrapper(union task_union *new) {
	unsigned int current_sp = 0;
	__asm__ __volatile__ (
			"stmfd	sp!, {r0-r12};"
			"mov 	%0, sp;"
			: "=r"(current_sp)
	);
	task_switch(new, current_sp);
	__asm__ __volatile__("ldmfd sp!, {r0-r12};");
}

/* Task switch */
void task_switch(union task_union *new, unsigned int last_sp) {
	unsigned int last_lr;
	__asm__ __volatile__ ("mov %0, lr" : "=r"(last_lr));
	
	struct task_struct * current_pcb = current();
	fl_page_table_entry * dir_new = get_DIR((struct task_struct *) new);
	fl_page_table_entry * dir_current = get_DIR(current_pcb);

	/* Change directory base and flushes TLB except for threads */	
	if (dir_new != dir_current) mmu_change_dir(dir_new);

	/* Save the kernel/user state. (User saved when entered to the kernel) */	
	current_pcb->kernel_sp = last_sp;
	current_pcb->kernel_lr = last_lr;
	
	__asm__ __volatile__ (
			/* set new kernel sp/lr */
			"mov	sp, %0;"
			"mov	lr, %1;"
			/* set new user sp/lr */
			"cps	#0x1F;" // SYS mode
			"mov	sp, %2;"
			"mov	lr, %3;"
			"cps	#0x13;"	// SVC mode
			"bx		lr;"
			: /* no output */
			: "r" (new->task.kernel_sp), "r" (new->task.kernel_lr),
			  "r" (new->task.user_sp),	 "r" (new->task.user_lr)
	);
}

/* Return new PID */
int getNewPID() {
	return ++lastPID;
}

/* Returns the task_struct at the head of the list */
struct task_struct *list_head_to_task_struct(struct list_head *l) {
	return list_entry(l,struct task_struct,list);
}

/* Returns a pointer to the current task_struct */
struct task_struct* current() {
	int ret_value=0;
	__asm__ __volatile__ (
		"mov %0, sp;"
		: "=g"(ret_value)
	);
	return (struct task_struct*)(ret_value&0xfffff000);
}

/* SCHEDULER */

/* Initialize RR scheduler */
void init_Sched_RR() {
	/* Scheduler RR selected*/
	sched_update_data = sched_update_data_RR;
	sched_change_needed = sched_change_needed_RR;
	sched_switch_process = sched_switch_process_RR;
	sched_update_queues_state = sched_update_queues_state_RR;
	rr_quantum = DEFAULT_RR_QUANTUM;

	struct task_struct * current_task = current();
	current_task->statistics.remaining_quantum = DEFAULT_RR_QUANTUM;
	current_task->process_state = ST_READY;
}

/* Update RR scheduler data */
void sched_update_data_RR() {
	--rr_quantum;

	struct task_struct * current_task = current();
	--(current_task->statistics.remaining_quantum);
	++(current_task->statistics.tics);
}

/* RR scheduler check variable */
int sched_change_needed_RR() {
	return rr_quantum == 0;
}

/* Task switch RR scheduler */
void sched_switch_process_RR() {
	struct list_head *task_list;
	struct task_struct * task;

	if (!circularbIsEmpty(&uart_read_buffer) && !list_empty(&keyboardqueue)) {
		task_list = list_first(&keyboardqueue);
		list_del(task_list);
		task = list_head_to_task_struct(task_list);
	}
	else if (!list_empty(&readyqueue)) {
		task_list = list_first(&readyqueue);
		list_del(task_list);
		task = list_head_to_task_struct(task_list);
	}
	else task = idle_task;

	task->statistics.remaining_quantum = DEFAULT_RR_QUANTUM;
	rr_quantum = DEFAULT_RR_QUANTUM;
	if (task != current()) {
		++task->statistics.cs;
		task->process_state = ST_RUN;
		current()->process_state = ST_READY;
		task_switch_wrapper((union task_union*)task);
	}
}

/* Update queues state RR scheduler */
void sched_update_queues_state_RR(struct list_head* ls, struct task_struct * task) {
	if (ls == &freequeue) task->process_state = ST_ZOMBIE;
	else if (ls == &readyqueue) task->process_state = ST_READY;
	else if (ls == &keyboardqueue) task->process_state = ST_BLOCKED;
	else task->process_state = ST_BLOCKED;

	if (task != idle_task) {
		if (ls == &keyboardqueue && task->kbinfo.keysread != 0) list_add(&task->list,ls);
		else list_add_tail(&task->list,ls);
	}
}
