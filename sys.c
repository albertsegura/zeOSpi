#include <cbuffer.h>
#include <devices.h>
#include <errno.h>
#include <interrupt.h>
#include <io.h>
#include <mm.h>
#include <mm_address.h>
#include <sched.h>
#include <sem.h>
#include <stats.h>
#include <system.h>
#include <timer.h>
#include <utils.h>
#include <gpio.h>

#define LECTURA 0
#define ESCRIPTURA 1

/* Check read/write fd function */
int check_fd(int fd, int permissions)
{
  if (fd != 1 && fd != 0) return -EBADF;
  if (fd == 1 && permissions!=ESCRIPTURA) return -EACCES;
  if (fd == 0 && permissions!=LECTURA) return -EACCES;
  return 0;
}

/* "Not implemented" syscall */
int sys_ni_syscall() {
	return -ENOSYS;
}

/* Syscall getpid, returns current PID */
int sys_getpid() {
	return current()->PID;
}

/* Syscall clone, thread creation */
int sys_clone(void (*function)(void), void *stack, unsigned int last_sp) {
	int PID;
	unsigned int pos_sp = 0;

	/* Variables initialization, get new task_struct from freequeue */
	if (list_empty(&freequeue)) return -ENTASK;
	struct list_head *new_list_pointer = list_first(&freequeue);
	list_del(new_list_pointer);
	struct task_struct * new_pcb = list_head_to_task_struct(new_list_pointer);
	struct task_struct * current_pcb = current();
	union task_union *new_stack = (union task_union*)new_pcb;
	pos_sp = ((unsigned int)last_sp-(unsigned int)current_pcb)/4;

	/* Copy of the stack and increment of the references to the directory/heap */
	copy_data(current_pcb, new_pcb, 4096);
	*(new_pcb->dir_count) += 1;
	*(new_pcb->pb_count) += 1;

	/* Set the state for this process to return to userspace function w/ stack
	 * going through kernelspace ret_from_for w/ kernelstack */
	new_pcb->kernel_sp = (unsigned int)&new_stack->stack[pos_sp];
	new_pcb->kernel_lr = (unsigned int)&ret_from_fork;
	new_pcb->user_sp = (unsigned int)stack;
	new_pcb->user_lr = (unsigned int)function; // we could try to go to exit
	new_stack->stack[pos_sp+10] = (unsigned int)function;

	/* Stats initialization */
	new_pcb->process_state = ST_READY;
	new_pcb->statistics.tics = 0;
	new_pcb->statistics.cs = 0;
	PID = getNewPID();
	new_pcb->PID = PID;

	/* Push to readyqueue to be scheduled */
	sched_update_queues_state(&readyqueue,new_pcb);

	return PID;

}

/* Syscall clone wrapper */
int sys_clone_wrapper(void (*function)(void), void *stack) {
	int ret, current_sp = 0;
	__asm__ __volatile__("mov %0, sp;" : "=r" (current_sp));
	ret = sys_clone(function, stack, current_sp);
	return ret;

}

/* Debug task_switch syscall */
int sys_DEBUG_tswitch() {

	sched_update_queues_state(&readyqueue,current());
	sched_switch_process();

	return 0;
}

/* Syscall fork, task creation */
int sys_fork(unsigned int last_sp) {
	int PID;
	unsigned int pos_sp = 0; // sp position relatively from the stack
	int pag, pb;
	int new_ph_pag;
	int frames[NUM_PAG_DATA];

	/* Variables initialization, get new task_struct from freequeue */
	if (list_empty(&freequeue)) return -ENTASK;
	struct list_head *new_list_pointer = list_first(&freequeue);
	list_del(new_list_pointer);
	struct task_struct * new_pcb = list_head_to_task_struct(new_list_pointer);
	struct task_struct * current_pcb = current();
	union task_union *new_stack = (union task_union*)new_pcb;
	pos_sp = ((unsigned int)last_sp-(unsigned int)current_pcb)/4;

	/* Get new frames for the process */
	for (pag=0; pag<NUM_PAG_DATA; pag++) {
		new_ph_pag=alloc_frame();
		if (new_ph_pag == -1) {
			while(pag != 0) free_frame(frames[--pag]); // rollback
			return -ENMPHP;
		}
		else frames[pag] = new_ph_pag;
	}

	/* Copy of the stack and get directory for the child */
	copy_data(current_pcb, new_pcb, 4096);
	allocate_page_dir(new_pcb);

	/* Copy code page tables, associate the new data frames */
	sl_page_table_entry * pt_usr_new = get_PT(new_pcb,1);
	sl_page_table_entry * pt_usr_current = get_PT(current_pcb,1);
	fl_page_table_entry * dir_current = get_DIR(current_pcb);

	/* CODE */
	for (pag=0;pag<NUM_PAG_CODE;pag++) {
		pt_usr_new[pag].entry = pt_usr_current[pag].entry;
	}

	/* DATA */
	for (pag=0;pag<NUM_PAG_DATA;pag++) {
		set_ss_pag(pt_usr_new,INIT_USR_DATA_PAG_D1+pag,frames[pag]);
	}

	/* Copy user data */
	/* Extra code to avoid selecting used pages. */
	int free_pag = PROC_FIRST_FREE_PAG_D1;
	for (pag=0; pag<NUM_PAG_DATA; pag++) {
			while (check_used_page(&pt_usr_current[free_pag]) && free_pag<TOTAL_PAGES_ENTRIES) free_pag++;

			if (free_pag == TOTAL_PAGES_ENTRIES) {
				if (pag > 0) {
					free_pag = PROC_FIRST_FREE_PAG_D1;
					--pag;
					mmu_change_dir(dir_current);
				}
				else return -ENEPTE;
			}
			else {
				set_ss_pag(pt_usr_current,free_pag,pt_usr_new[INIT_USR_DATA_PAG_D1+pag].bits.pbase_addr);
				copy_data((void *)((PAG_LOG_INIT_DATA_P0+pag)<<12),	(void *)((0x100+free_pag)<<12), PAGE_SIZE);
				del_ss_pag(pt_usr_current, free_pag);

				free_pag++;
			}
	}

	/* TLB flush */
	mmu_change_dir(dir_current);

	/* Copy Heap data */
	get_newpb(new_pcb);
	*(new_pcb->program_break) = *(current_pcb->program_break);
	pb = *(current_pcb->program_break);
	free_pag = PAGE(pb+(1<<OFFSET_BITS));
	for (pag=USR_P_HEAPSTART; pag < PAGE(pb) || ( pag == PAGE(pb) && (0 != OFFSET(pb)) );pag++) {

		while(check_used_page(&pt_usr_current[free_pag]) && free_pag<TOTAL_PAGES_ENTRIES) free_pag++;

		if (free_pag == TOTAL_PAGES_ENTRIES) {
			if (pag > USR_P_HEAPSTART) {
				free_pag = PAGE(pb+(1<<OFFSET_BITS));
				--pag;
				mmu_change_dir(dir_current);
			}
			else return -ENEPTE;
		}
		else {
			new_ph_pag=alloc_frame();
			if (new_ph_pag == -1) {
				for(pag = pag-1; pag >= USR_P_HEAPSTART; pag--) free_frame(pt_usr_new[pag].bits.pbase_addr);
				return -ENMPHP;
			}

			/* Association to the new task */
			set_ss_pag(pt_usr_new,pag,new_ph_pag);

			/* Temporally association to copy the page on the current task */
			set_ss_pag(pt_usr_current,free_pag,pt_usr_new[pag].bits.pbase_addr);
			copy_data((void *)((pag)<<12),	(void *)(free_pag<<12), PAGE_SIZE);
			del_ss_pag(pt_usr_current, free_pag);

			free_pag++;
		}
	}

	/* TLB flush */
	mmu_change_dir(dir_current);

	/* Setting the returning state */
	new_pcb->kernel_sp = (unsigned int)&new_stack->stack[pos_sp];
	new_pcb->kernel_lr = (unsigned int)&ret_from_fork;
	new_pcb->user_sp = current_pcb->user_sp;
	new_pcb->user_lr = current_pcb->user_lr;

	/* Stats initialization */
	new_pcb->process_state = ST_READY;
	new_pcb->statistics.tics = 0;
	new_pcb->statistics.cs = 0;
	PID = getNewPID();
	new_pcb->PID = PID;

	/* Push to readyqueue to be scheduled */
	sched_update_queues_state(&readyqueue,new_pcb);

	return PID;
}

/* Syscall fork wrapper */
int sys_fork_wrapper() {
	int current_sp = 0;
	__asm__ __volatile__("mov %0, sp;" : "=r" (current_sp));
	return sys_fork(current_sp);
}

/* Syscall exit, kills current process */
void sys_exit() {
	int pag;
	struct task_struct * current_pcb = current();
	sl_page_table_entry * pt_current = get_PT(current_pcb,1);

	/* Free DATA & HEAP region */
	if (*(current_pcb->dir_count) == 1) {
		for (pag=PROC_FIRST_FREE_PAG_D1;pag<TOTAL_PAGES_ENTRIES ;pag++){
			free_frame(pt_current[pag].bits.pbase_addr);
		}
	}
	*(current_pcb->dir_count) -= 1;
	*(current_pcb->pb_count) -= 1;

	sched_update_queues_state(&freequeue,current());
	sched_switch_process();
}

/* Syscall write */
int sys_write(int fd, char * buffer, int size) {
	char buff[4];
	int ret = 0;

	ret = check_fd(fd,ESCRIPTURA);
	if (ret != 0) 		return ret;
	if (buffer == NULL)	return -EPNULL;
	if (size <= 0) 		return -ESIZEB;
	if (access_ok(VERIFY_READ, buffer, size) == 0) return -ENACCB;

	while (size > 4) {
		copy_from_user(buffer, buff, 4);
		ret += sys_write_uart(buff,4);
		buffer += 4;
		size -= 4;
	}
	copy_from_user(buffer, buff, size);
	ret += sys_write_uart(buff,size);

	return ret;
}

/* Syscall read */
int sys_read(int fd, char * buffer, int size) {
	int ret = 0;

	ret = check_fd(fd,LECTURA);
	if (ret != 0) 		return ret;
	if (buffer == NULL)	return -EPNULL;
	if (size <= 0) 		return -ESIZEB;
	if (access_ok(VERIFY_WRITE, buffer, size) == 0) return -ENACCB;

	ret = sys_read_uart(buffer,size);

	return ret;
}

/* Syscall led */
void sys_led(int state) {
	if (state) gpio_set_led_on();
	else gpio_set_led_off();
}

/* Syscall gettime */
unsigned int sys_gettime() {
	return clock_get_time();
}

/* Syscall get_stats */
int sys_get_stats(int pid, struct stats *st) {
	struct task_struct * desired;
	int found;

	if (access_ok(VERIFY_WRITE,st,12) == 0) return -ENACCB;

	found = getStructPID(pid, &readyqueue, &desired);
	if (!found) found = getStructPID(pid, &keyboardqueue, &desired);
	if (found) copy_to_user(&desired->statistics,st,12);

	else return -ENSPID;
	return 0;
}


/* SEMAPHORES */

/* Syscall semaphore init, init n_sem semaphore */
int sys_sem_init(int n_sem, unsigned int value) {
	int ret = 0;

	if (n_sem < 0 || n_sem >= SEM_SIZE) return -EINVSN;
	if (sem_array[n_sem].pid_owner != -1) return -ESINIT;

	sem_array[n_sem].pid_owner = current()->PID;
	sem_array[n_sem].value = value;
	INIT_LIST_HEAD(&sem_array[n_sem].semqueue);
	return ret;
}

/* Syscall semaphore wait, waits on n_sem semaphore */
int sys_sem_wait(int n_sem) {
	int ret = 0;

	if (n_sem < 0 || n_sem >= SEM_SIZE) return -EINVSN;
	if (sem_array[n_sem].pid_owner == -1) return -ENINIT;

	if (sem_array[n_sem].value <= 0) {
		sched_update_queues_state(&sem_array[n_sem].semqueue,current());
		sched_switch_process();
	}
	else sem_array[n_sem].value--;

	/* We tell the user if the sem was destroyed or not */
	if (sem_array[n_sem].pid_owner == -1) ret = -ESDEST;

	return ret;
}

/* Syscall semaphore signal, send signal to n_sem semaphore */
int sys_sem_signal(int n_sem) {
	int ret = 0;

	if (n_sem < 0 || n_sem >= SEM_SIZE) return -EINVSN;
	if (sem_array[n_sem].pid_owner == -1) return -ENINIT;

	if(list_empty(&sem_array[n_sem].semqueue)) sem_array[n_sem].value++;
	else {
		struct list_head *task_list = list_first(&sem_array[n_sem].semqueue);
		list_del(task_list);
		struct task_struct * semtask = list_head_to_task_struct(task_list);
		sched_update_queues_state(&readyqueue,semtask);
	}

	return ret;
}

/* Syscall semaphore destroy, destroy n_sem semaphore */
int sys_sem_destroy(int n_sem) {
	int ret = 0;

	if (n_sem < 0 || n_sem >= SEM_SIZE) return -EINVSN;
	if (sem_array[n_sem].pid_owner == -1) return -ENINIT;

	if (current()->PID == sem_array[n_sem].pid_owner) {
		sem_array[n_sem].pid_owner = -1;
		while (!list_empty(&sem_array[n_sem].semqueue)) {
			struct list_head *task_list = list_first(&sem_array[n_sem].semqueue);
			list_del(task_list);
			struct task_struct * semtask = list_head_to_task_struct(task_list);
			sched_update_queues_state(&readyqueue,semtask);
		}
	}
	else ret = -ESNOWN;

	return ret;
}

/* Syscall sbrk, dynamic memory */
void *sys_sbrk(int increment) {
	int i;
	struct task_struct * current_pcb = current();
	unsigned int pb = *(current_pcb->program_break);
	void * ret  = (void *)*(current_pcb->program_break);
	fl_page_table_entry * dir_current = get_DIR(current_pcb);
	sl_page_table_entry * pt_current = get_PT(current_pcb,1);

	if (increment > 0) {
		int end = ((pb+increment)>>OFFSET_BITS)-(1<<PAGE_BITS);

		if (end < TOTAL_PAGES_ENTRIES) { /* Lower limit of the HEAP */
			for(i = PAGE(pb); i < end || ( i==end && (0!=OFFSET((pb+increment))) ); ++i) {
				if (!check_used_page(&pt_current[i])) {
					int new_ph_pag=alloc_frame();
					if (new_ph_pag == -1) return (void *)-ENMPHP;
					set_ss_pag(pt_current,i,new_ph_pag);
				}
			}
		}
		else return (void *)-ENOMEM;
	}
	else if (increment < 0) {
		int new_pb = ((pb+increment)>>OFFSET_BITS)-(1<<PAGE_BITS);;

		if (new_pb >= USR_P_HEAPSTART) { /* Upper limit of the HEAP */
			for(i = PAGE(pb); i > new_pb || ( i==new_pb && (0==OFFSET((pb+increment))) ); --i) {
				free_frame(pt_current[i].bits.pbase_addr);
				del_ss_pag(pt_current, i);
			}
			mmu_change_dir(dir_current);
		}
		else return (void *)-EHLIMI;
	}

	*(current_pcb->program_break) += increment;

	return ret;
}



