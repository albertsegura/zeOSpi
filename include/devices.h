#ifndef __DEVICES_H__
#define	__DEVICES_H__

#include <types.h>

void interrupt_uart_routine();

int sys_write_uart(char *buffer, int size);
int sys_read_uart(char *buffer, int size);

#endif /* __DEVICES_H__*/
