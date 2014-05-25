/*
 * libc.c 
 */

#include <libc.h>

#include <types.h>

#include <errno.h>

void itoa(int a, char *b) {
	int i, i1;
	char c;

	if (a==0) {
		b[0]='0';
		b[1]=0;
		return;
	}

	i=0;
	while (a>0) {
		b[i]=(a%10)+'0';
		a=a/10;
		i++;
	}

	for (i1=0; i1<i/2; i1++) {
		c=b[i1];
		b[i1]=b[i-i1-1];
		b[i-i1-1]=c;
	}
	b[i]=0;
}

int strlen(char *a) {
	int i;

	i=0;
	while (a[i]!=0) i++;

	return i;
}


/* Wrapper de la Syscall Write */
int write (int fd, char *buffer, int size) {
	int ret;
	__asm__ volatile(
		"mov %%r0, %1;"
		"mov %%r1, %2;"
		"mov %%r2, %3;"
		"mov %%r7, %4;"
		"svc 0x0;"
		"mov %0, %%r0;"	// TODO fer per altres syscalls
		:"=r" (ret), 				// %0, resultat a ret
		"+r" (fd),					// %1, parameter 1
		"+r" (buffer),				// %2, parameter 2
		"+r" (size)					// %3, parameter 3.
		:"r" (4)					// %4, sys_call_table index
		:"r0", "r1", "r2", "r7"		// We tell the compiler the registers modified
	);
	if (ret < 0) {
		errno = -ret;
		ret = -1;
	}
	return ret;
}

/* Wrapper de la Syscall Gettime */
unsigned int gettime() {
	int ret;
	__asm__ volatile(
		"mov %%r7, %1;"
		"svc 0x0;"
		:"=r" (ret)
		:"r"  (10)
	 	:"r7"
	);
	return ret;
}

/* Wrapper de la Syscall GetPid */
int getpid(void) {
	int ret;
	__asm__ volatile(
		"mov %%r7, %1;"
		"svc 0x0;"
		:"=r" (ret)
		:"r"  (20)
		:"r7"
	);
	return ret;
}

/* Wrapper de la Syscall Fork */
int fork() {
	int ret;
	__asm__ volatile(
		"mov %%r7, %1;"
		"svc 0x0;"
		:"=r" (ret)
		:"r"  (2)
		:"r7"
	);
	if (ret < 0) {
		errno = -ret;
		ret = -1;
	}
	return ret;
}

/* Wrapper de la Syscall de Debug sys_DEBUG_tswitch */
int debug_task_switch() {
	int ret;
	__asm__ volatile(
		"mov %%r7, %1;"
		"svc 0x0;"
		:"=r" (ret)
		:"r"  (9)
		:"r7"
	);
	return ret;
}

/* Wrapper de la Syscall Exit */
void exit() {
	__asm__ volatile(
		"mov %%r7, %0;"
		"svc 0x0;"
		:
		:"r"  (1)
		:"r7"
	);
}

/* Wrapper de la Syscall get_stats */
int get_stats(int pid, struct stats *st) {
	int ret;
	__asm__ volatile(
		"mov %%r0, %1;"
		"mov %%r1, %2;"
		"mov %%r7, %3;"
		"svc 0x0;"
		:"=r" (ret),
		"+r" (pid),
		"+r" (st)
		:"r" (35)
		:"r0", "r1", "r7"
	);
	if (ret < 0) {
		errno = -ret;
		ret = -1;
	}
	return ret;
}

/* Wrapper de la Syscall clone */
int clone (void (*function)(void), void *stack) {
/*	function: starting address of the function to be executed by the new process
 * stack : starting address of a memory region to be used as a stack
 * returns: -1 if error or the pid of the new lightweight process ID if OK
 */
	int ret;
	__asm__ volatile(
		"mov %%r0, %1;"
		"mov %%r1, %2;"
		"mov %%r7, %3;"
		"svc 0x0;"
		:"=r" (ret),
		"+r" (function),
		"+r" (stack)
		:"r" (3)
		:"r0", "r1", "r7"
	);
	if (ret < 0) {
		errno = -ret;
		ret = -1;
	}
	return ret;
}


/* Wrapper de la Syscall Read */
int read (int fd, char *buffer, int size) {
	int ret;
	__asm__ volatile(
		"mov %%r0, %1;"
		"mov %%r1, %2;"
		"mov %%r2, %3;"
		"mov %%r7, %4;"
		"svc 0x0;"
		:"=r" (ret),
		"+r" (fd),
		"+r" (buffer),
		"+r" (size)
		:"r"  (5)
		:"r0", "r1", "r2", "r7"
	);
	if (ret < 0) {
		errno = -ret;
		ret = -1;
	}
	return ret;
}

// TODO sem & sbrk syscalls


