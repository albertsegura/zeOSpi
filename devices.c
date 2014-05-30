/*
 * devices.c -
 */

#include <cbuffer.h>
#include <io.h>
#include <list.h>
#include <mm.h>
#include <mm_address.h>
#include <sched.h>
#include <system.h>
#include <uart.h>
#include <utils.h>
#include <devices.h>

void interrupt_uart_routine() {
	char data, aux;

	data = uart_get_byte();

	/* S'escriu si el Buffer no està ple */
	if (circularbIsFull(&uart_read_buffer)) circularbRead(&uart_read_buffer, &aux);
	circularbWrite(&uart_read_buffer,&data);

	// TODO aqui hi havia codi de gestió mes avançat
}


int sys_read_uart(char *buffer, int size) {
	int i=0;
	char read;
	struct task_struct * current_pcb = current();

	current_pcb->kbinfo.keystoread = size;
	current_pcb->kbinfo.keybuffer = buffer;
	current_pcb->kbinfo.keysread = 0;

	if (!list_empty(&keyboardqueue)) {
		sched_update_queues_state(&keyboardqueue,current(),0);
		sched_switch_process();
	}

	while (current_pcb->kbinfo.keystoread > 0) {

		if (!circularbIsEmpty(&uart_read_buffer)){
			for (i=current_pcb->kbinfo.keystoread; i>0 && !circularbIsEmpty(&uart_read_buffer); i--) {
				circularbRead(&uart_read_buffer,&read);
				copy_to_user(&read, current_pcb->kbinfo.keybuffer, 1);
				current_pcb->kbinfo.keybuffer++;
			}
			current_pcb->kbinfo.keysread += current_pcb->kbinfo.keystoread-i;
			current_pcb->kbinfo.keystoread = i;
		}

		if (current_pcb->kbinfo.keystoread > 0){
			// Insert in the front of the queue
			sched_update_queues_state(&keyboardqueue,current(),1);
			sched_switch_process();
		}
	}

	return current_pcb->kbinfo.keysread;
}

int sys_write_uart(char *buffer,int size) {
	int i;

	for (i=0; i<size; i++) printc(buffer[i]);

	return size;
}

