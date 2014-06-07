#ifndef __SYSTEM_H__
#define __SYSTEM_H__

#include <cbuffer.h>
#include <sem.h>
#include <types.h>

extern Circular_Buffer uart_read_buffer;
extern Sem sem_array[SEM_SIZE];

#endif  /* __SYSTEM_H__ */
