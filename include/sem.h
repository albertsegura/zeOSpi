#ifndef __SEMAPHORE__
#define __SEMAPHORE__

#include <list.h>

typedef struct {
		int id;
		unsigned int value;
		int pid_owner;
		struct list_head semqueue;
}Sem;

#endif /* __SEMAPHORE__ */
