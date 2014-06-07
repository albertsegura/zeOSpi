#ifndef __UTILS_H__
#define __UTILS_H__


#define USR_MODE	0x10
#define FIQ_MODE	0x11
#define IRQ_MODE	0x12
#define SVC_MODE	0x13
#define ABT_MODE	0x17
#define UND_MODE	0x1B
#define SYS_MODE	0x1F
#define MON_MODE	0x16

#define VERIFY_READ		0
#define VERIFY_WRITE	1

#define min(a,b) (a<b?a:b)

#define read_cpsr() ({ \
	unsigned int _tmp=0;	\
	__asm__ __volatile__ ("MRS %0, cpsr;" : "=r"(_tmp)); \
	_tmp; });

#define write_cpsr(_tmp) {__asm__ __volatile__ ("MSR cpsr, %0;" : : "r"(_tmp));};


int access_ok(int type, const void *addr, unsigned long size);
void copy_data(void *start, void *dest, int size);
int copy_from_user(void *start, void *dest, int size);
int copy_to_user(void *start, void *dest, int size);

inline void set_address_to(unsigned int address, unsigned int value);
inline unsigned int get_value_from(unsigned int address);

void delay();



#endif
