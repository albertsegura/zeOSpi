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

int sys_clone (void (*function)(void), void *stack) {
	return 0;
}

int sys_fork() {
	int PID;
	int current_ebp = 0;
	unsigned int pos_ebp = 0; // posició del ebp en la stack: new/current_stack->stack[pos_ebp]
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

	// TODO imitar per a arm
	/* Càlcul de pos_ebp */
	/*__asm__ __volatile__(
	  		"mov %%ebp,%0;"
	  		: "=r" (current_ebp)
	  );
	pos_ebp = ((unsigned int)current_ebp-(unsigned int)current_pcb)/4;*/

	/* Obtenció dels frames per al nou procés */
	for (pag=0;pag<NUM_PAG_DATA;pag++){
		new_ph_pag=alloc_frame();
		if (new_ph_pag == -1) {
			while(pag != 0) free_frame(frames[--pag]); // rollback
			return -ENMPHP;
		}
		else frames[pag] = new_ph_pag;
	}

	/* Copia del Stack, i restauració del directori del fill*/
	copy_data(current_pcb, new_pcb, 4096);
	// TODO allocate_page_dir(new_pcb);

	/* Punt d.i: Copia de les page tables de codi, i assignació de
	 * 						frames per a les dades	*/
	sl_page_table_entry * pt_usr_new = get_PT(new_pcb,1);
	sl_page_table_entry * pt_usr_current = get_PT(current_pcb,1);
	fl_page_table_entry * dir_current = get_DIR(current_pcb);

	/* CODE */
	for (pag=0;pag<NUM_PAG_CODE;pag++) {
		pt_usr_new[PAG_LOG_INIT_CODE_P0-NUM_PAG_KERNEL+pag].entry
			= pt_usr_current[PAG_LOG_INIT_CODE_P0-NUM_PAG_KERNEL+pag].entry;
	}

	/* DATA */
	for (pag=0;pag<NUM_PAG_DATA;pag++) {
		set_ss_pag(pt_usr_new,PAG_LOG_INIT_DATA_P0-NUM_PAG_KERNEL+pag,frames[pag]);
	}

	// TODO copia de la zona de dades

	/* Flush de la TLB */
	// TODO set_cr3(dir_current);

	// TODO gestió copia memoria dinàmica

	PID = getNewPID();
	new_pcb->PID = PID;

	/* Punt f i g */
	// eax del Save_all (Serà el pid de retorn del fill)
	new_stack->stack[pos_ebp+8] = 0;
	// Construint l'enllaç dinamic fent que el esp apunti al ebp guardat
	new_stack->task.kernel_esp = (unsigned int)&new_stack->stack[pos_ebp];
	// Modificant la funció a on retornarà
	//new_stack->stack[pos_ebp+1] = (unsigned int)&ret_from_fork; // TODO veure com implementar-ho

	/* Inicialització estadistica */
	new_stack->task.process_state = ST_READY;
	new_stack->task.statistics.tics = 0;
	new_stack->task.statistics.cs = 0;

	/* Punt h */
	sched_update_queues_state(&readyqueue,new_pcb);


	return PID;
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



