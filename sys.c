/*
 * sys.c - Syscalls implementation
 */

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

int check_fd(int fd, int permissions)
{
  if (fd != 1 && fd != 0) return -EBADF;
  if (fd == 1 && permissions!=ESCRIPTURA) return -EACCES;
  if (fd == 0 && permissions!=LECTURA) return -EACCES;
  return 0;
}

int sys_ni_syscall() {
	return -ENOSYS;
}

int sys_getpid() {
	return current()->PID;
}

int sys_clone(void (*function)(void), void *stack, unsigned int last_sp) {
	int PID;
	unsigned int pos_sp = 0;

	/* Obtenció d'una task_struct nova de la freequeue */
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

	PID = getNewPID();
	new_pcb->PID = PID;

	/* Set the state for this process to return to userspace function w/ stack
	 * going through kernelspace ret_from_for w/ kernelstack */
	new_pcb->kernel_sp = (unsigned int)&new_stack->stack[pos_sp];
	new_pcb->kernel_lr = (unsigned int)&ret_from_fork;
	new_pcb->user_sp = (unsigned int)stack;
	new_pcb->user_lr = (unsigned int)function; // we could try to go to exit
	new_stack->stack[pos_sp+10] = (unsigned int)function;

	/* Inicialització estadistica */
	new_pcb->process_state = ST_READY;
	new_pcb->statistics.tics = 0;
	new_pcb->statistics.cs = 0;

	/* Push to readyqueue to be scheduled */
	sched_update_queues_state(&readyqueue,new_pcb);

	return PID;

}

int sys_clone_wrapper(void (*function)(void), void *stack) {
	int ret, current_sp = 0;
	__asm__ __volatile__("mov %0, sp;" : "=r" (current_sp));
	ret = sys_clone(function, stack, current_sp);
	return ret;

}

int sys_DEBUG_tswitch() {

	sched_update_queues_state(&readyqueue,current());
	sched_switch_process();

	return 0;
}

int sys_fork(unsigned int last_sp) {
	int PID;
	unsigned int pos_sp = 0; // sp position relatively from the stack
	int pag;
	int new_ph_pag;
	int frames[NUM_PAG_DATA];

	/* Obtenció d'una task_struct nova de la freequeue */
	if (list_empty(&freequeue)) return -ENTASK;
	struct list_head *new_list_pointer = list_first(&freequeue);
	list_del(new_list_pointer);
	struct task_struct * new_pcb = list_head_to_task_struct(new_list_pointer);
	struct task_struct * current_pcb = current();
	union task_union *new_stack = (union task_union*)new_pcb;


	pos_sp = ((unsigned int)last_sp-(unsigned int)current_pcb)/4;

	/* Obtenció dels frames per al nou procés */
	for (pag=0;pag<NUM_PAG_DATA;pag++){
		new_ph_pag=alloc_frame();
		if (new_ph_pag == -1) {
			while(pag != 0) free_frame(frames[--pag]); // rollback
			return -ENMPHP;
		}
		else frames[pag] = new_ph_pag;
	}

	/* Copia del Stack, i obtenció del directori del fill*/
	copy_data(current_pcb, new_pcb, 4096);
	allocate_page_dir(new_pcb);

	/* Punt d.i: Copia de les page tables de codi, i assignació de
	 * 						frames per a les dades	*/
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

	/* Punt d.ii */
	/* Gestió extra per evitar problemes amb Memoria dinàmica:
	 * 	- Busquem entrades a la page_table lliures, en comptes de 20 directes
	 * 	- Si arribem al limit sense haver-ne trobat ni una retornem error, doncs
	 * 		no es pot fer el fork per falta de page tables entry al pare.
	 * 	- Si arribem al limit però hem pogut fer almenys una copia:
	 * 		flush a la TLB i tornem a buscar desde el principi.
	 * */
	int free_pag = PROC_FIRST_FREE_PAG_D1;
	for (pag=0;pag<NUM_PAG_DATA;pag++){
			while (check_used_page(&pt_usr_current[free_pag]) && free_pag<TOTAL_PAGES_ENTRIES) free_pag++;

			if (free_pag == TOTAL_PAGES_ENTRIES) {
				if (pag != 0) {
					free_pag = PROC_FIRST_FREE_PAG_D1;
					--pag;
					// TLB flush
					mmu_change_dir(dir_current);
				}
				else return -ENEPTE;
			}
			else {
				/* d.ii.A: Assignació de noves pàgines logiques al procés actual, corresponents
				 * 					a les pàgines físiques obtingudes per al procés nou	*/
				set_ss_pag(pt_usr_current,free_pag,pt_usr_new[INIT_USR_DATA_PAG_D1+pag].bits.pbase_addr);

				/* d.ii.B: Copia de l'espai d'usuari del proces actual al nou */
				copy_data((void *)((PAG_LOG_INIT_DATA_P0+pag)<<12),	(void *)((0x100+free_pag)<<12), PAGE_SIZE);

				/* d.ii.C: Desassignació de les pagines en el procés actual */
				del_ss_pag(pt_usr_current, free_pag);

				free_pag++;
			}
	}

	/* Flush de la TLB */
	mmu_change_dir(dir_current);


	get_newpb(new_pcb);
	*(new_pcb->program_break) = *(current_pcb->program_break);

	/* Copia de la zona HEAP, similar a la copia de pagines de dades */
	free_pag = ((*(current_pcb->program_break)>>12)&0xFF)+1;
	for (pag=USR_P_HEAPSTART; pag < ((*(current_pcb->program_break)>>12)&0xFF) ||
					( pag == ((*(current_pcb->program_break)>>12)&0xFF) && (0 != (*(current_pcb->program_break) & (PAGE_SIZE-1))) );pag++) {
			while(!check_used_page(&pt_usr_current[free_pag]) && free_pag<TOTAL_PAGES_ENTRIES) free_pag++;

			if (free_pag == TOTAL_PAGES_ENTRIES) {
				if (pag != 0) {
					free_pag = ((*(current_pcb->program_break)>>12)&0xFF)+1;
					--pag;
					mmu_change_dir(dir_current);
				}
				else return -ENEPTE;
			}
			else {
				/* Obtenció del frame pel heap del fill */
				new_ph_pag=alloc_frame();
				if (new_ph_pag == -1) {
					pag--;
					while(pag >= USR_P_HEAPSTART) free_frame(pt_usr_new[pag--].bits.pbase_addr);
					return -ENMPHP;
				}

				/* Assignació del frame nou, al procés fill */
				set_ss_pag(pt_usr_new,pag,new_ph_pag);

				/* Copia del Heap: es necessari posar el frame en el pt del pare per a fer-ho */
				set_ss_pag(pt_usr_current,free_pag,pt_usr_new[pag].bits.pbase_addr);
				copy_data((void *)((pag)<<12),	(void *)(free_pag<<12), PAGE_SIZE);
				del_ss_pag(pt_usr_current, free_pag);

				free_pag++;
			}
	}
	mmu_change_dir(dir_current);

	PID = getNewPID();
	new_pcb->PID = PID;

	/* Punt f i g */
	new_pcb->kernel_sp = (unsigned int)&new_stack->stack[pos_sp];
	new_pcb->kernel_lr = (unsigned int)&ret_from_fork;
	new_pcb->user_sp = USER_SP;
	new_pcb->user_lr = current_pcb->user_lr;

	/* Inicialització estadistica */
	new_pcb->process_state = ST_READY;
	new_pcb->statistics.tics = 0;
	new_pcb->statistics.cs = 0;

	/* Push to readyqueue to be scheduled */
	sched_update_queues_state(&readyqueue,new_pcb);

	return PID;
}

int sys_fork_wrapper() {
	int current_sp = 0;
	__asm__ __volatile__("mov %0, sp;" : "=r" (current_sp));
	return sys_fork(current_sp);
}

void sys_exit() {
	/* Punt a */
	int pag;
	struct task_struct * current_pcb = current();
	sl_page_table_entry * pt_current = get_PT(current_pcb,1);

	/* Allibera les 20 de Data i la resta (HEAP), a partir de l'entrada 256+8 */
	if (*(current_pcb->dir_count) == 1) {
		for (pag=PROC_FIRST_FREE_PAG_D1;pag<TOTAL_PAGES_ENTRIES ;pag++){
			free_frame(pt_current[pag].bits.pbase_addr);
		}
	}
	*(current_pcb->dir_count) -= 1;
	*(current_pcb->pb_count) -= 1;

	// TODO sem

	/* Punt b */
	sched_update_queues_state(&freequeue,current());
	sched_switch_process();
}


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

void sys_led(int state) {
	if (state) gpio_set_led_on();
	else gpio_set_led_off();
}

unsigned int sys_gettime() {
	return clock_get_time();
}

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
int sys_sem_init(int n_sem, unsigned int value) {
	int ret = 0;

	if (n_sem < 0 || n_sem >= SEM_SIZE) return -EINVSN;
	if (sem_array[n_sem].pid_owner != -1) return -ESINIT;
	sem_array[n_sem].pid_owner = current()->PID;
	sem_array[n_sem].value = value;
	INIT_LIST_HEAD(&sem_array[n_sem].semqueue);
	return ret;
}

int sys_sem_wait(int n_sem) {
	int ret = 0;
	if (n_sem < 0 || n_sem >= SEM_SIZE) return -EINVSN;
	if (sem_array[n_sem].pid_owner == -1) return -ENINIT;
	if (sem_array[n_sem].value <= 0) {
		sched_update_queues_state(&sem_array[n_sem].semqueue,current());
		sched_switch_process();
	}
	else sem_array[n_sem].value--;

	/* Hem de mirar si després de l'espera en la cua
	 * s'ha destruit el semafor o no, per indicar-ho al usuari*/
	if (sem_array[n_sem].pid_owner == -1) ret = -ESDEST;

	return ret;
}

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



