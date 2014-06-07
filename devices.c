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

/* Interrupt uart routine */
void interrupt_uart_routine() {
	char data;

	data = uart_get_byte();

	/* If the buffer is full, the data is lost */
	circularbWrite(&uart_read_buffer,&data);
}

/* Uart syscall read */
int sys_read_uart(char *buffer, int size) {
	int i=0;
	char read;
	struct task_struct * current_pcb = current();

	current_pcb->kbinfo.keystoread = size;
	current_pcb->kbinfo.keybuffer = buffer;
	current_pcb->kbinfo.keysread = 0;

	/* If someone else is waiting we wait */
	if (!list_empty(&keyboardqueue)) {
		sched_update_queues_state(&keyboardqueue,current());
		sched_switch_process();
	}

	/* Now we are the task at the front of the queue */
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
			/* Inserted at the front of the queue */
			sched_update_queues_state(&keyboardqueue,current());
			sched_switch_process();
		}
	}

	return current_pcb->kbinfo.keysread;
}

/* Uart syscall write */
int sys_write_uart(char *buffer,int size) {
	int i;

	for (i=0; i<size; i++) printc(buffer[i]);

	return size;
}

