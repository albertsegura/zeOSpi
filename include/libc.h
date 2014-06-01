/*
 * libc.h - macros per fer els traps amb diferents arguments
 *          definició de les crides a sistema
 */
 
#ifndef __LIBC_H__
#define __LIBC_H__

#include <stats.h>

void itoa(int a, char *b);
int strlen(char *a);

int write(int fd, char *buffer, int size);
int read (int fd, char *buffer, int size);
unsigned int gettime();
int getpid();
int fork();
int debug_task_switch();
void exit();
int get_stats(int pid, struct stats *st);
int clone (void (*function)(void), void *stack);
int sem_init (int n_sem, unsigned int value);
int sem_wait (int n_sem);
int sem_signal (int n_sem);
int sem_destroy (int n_sem);
void *sbrk (int increment);
void change_led(int status);

#endif  /* __LIBC_H__ */
