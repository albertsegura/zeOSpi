/*
 * sys.c - Syscalls implementation
 */
#include <devices.h>
#include <utils.h>
#include <io.h>
#include <mm.h>
#include <mm_address.h>
#include <stats.h>
#include <sched.h>
#include <errno.h>
#include <timer.h>
#include <cbuffer.h>
#include <interrupt.h>

#define LECTURA 0
#define ESCRIPTURA 1

int check_fd(int fd, int permissions) {
  if (fd!=1) return -EBADF;
  if (permissions!=ESCRIPTURA) return -EACCES;
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


	/* Copia del Stack, actualització del contador de punters del directori*/
	copy_data(current_pcb, new_pcb, 4096);
	*(new_pcb->dir_count) += 1;
	*(new_pcb->pb_count) += 1;

	PID = getNewPID();
	new_pcb->PID = PID;

	/* Construint l'enllaç dinàmic fent que el esp apunti al ebp guardat */
	new_pcb->kernel_sp = (unsigned int)&new_stack->stack[pos_sp];
	/* @ retorn estàndard: Restore ALL + iret */
	new_pcb->kernel_lr = (unsigned int)&ret_from_fork;
	new_pcb->user_sp = stack;
	new_pcb->user_lr = function; // TODO fer anar a la exit()?

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
	/*struct list_head *new_list_task = list_first(&readyqueue);
	list_del(new_list_task);
	struct task_struct * new_task = list_head_to_task_struct(new_list_task);
	struct task_struct * current_task = current();

	list_add_tail(&current_task->list,&readyqueue);

	task_switch((union task_union*)new_task);*/

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
					//set_cr3(dir_current);
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
	// TODO set_cr3(dir_current);

	// TODO gestió copia memoria dinàmica HEAP

	PID = getNewPID();
	new_pcb->PID = PID;

	/* Punt f i g */
	// Construint l'enllaç dinamic fent que el esp apunti al ebp guardat
	new_pcb->kernel_sp = (unsigned int)&new_stack->stack[pos_sp];
	// Modificant la funció a on retornarà
	new_pcb->kernel_lr = (unsigned int)&ret_from_fork;
	new_pcb->user_sp = 0x11c000-0x4; // TODO set define
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

unsigned int sys_gettime() {
	return clock_get_time();
}

int sys_get_stats(int pid, struct stats *st) {
	return 0;	struct task_struct * desired;
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
	return 0;
}

int sys_sem_wait(int n_sem) {
	return 0;
}

int sys_sem_signal(int n_sem) {
	return 0;
}

int sys_sem_destroy(int n_sem) {
	return 0;
}

void *sys_sbrk(int increment) {
	return 0;
}



