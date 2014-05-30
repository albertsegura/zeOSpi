#ifndef DEVICES_H__
#define  DEVICES_H__

#include <types.h>

void interrupt_uart_routine();

int sys_write_uart(char *buffer, int size);
int sys_read_uart(char *buffer, int size);

#endif /* DEVICES_H__*/
