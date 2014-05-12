#ifndef UTILS_H
#define UTILS_H

void copy_data(void *start, void *dest, int size);
int copy_from_user(void *start, void *dest, int size);
int copy_to_user(void *start, void *dest, int size);

inline void set_address_to(unsigned int address, unsigned int value);
inline unsigned int get_value_from(unsigned int address);

void delay(void);

#define VERIFY_READ		0
#define VERIFY_WRITE	1
int access_ok(int type, const void *addr, unsigned long size);

#define min(a,b)	(a<b?a:b)

#define read_cpsr() ({ \
	unsigned int _tmp=0;	\
	__asm__ __volatile__ ("MRS %0, cpsr;" : "=r"(_tmp)); \
	_tmp; });

#define write_cpsr(_tmp) {__asm__ __volatile__ ("MSR cpsr, %0;" : : "r"(_tmp));};

/*
#define read_copr(_CRn,_Op1,_CRm,_Op2) ({ \
	unsigned int _tmp=0;	\
	__asm__ __volatile__ ("MRC P15, _Op1,  %0,  _CRn, _CRm, _Op2;" : "=r"(_tmp)); \
	_tmp; });
*/

#define set_world_usr() {\
	unsigned int _tmp = read_cpsr();\
	_tmp = (_tmp & ~(0x1F));\
	_tmp = (_tmp | 0x10);\
	write_cpsr(_tmp); };

#define set_world_fiq() {\
	unsigned int tmp = read_cpsr();\
	tmp = (tmp & ~(0x1F));\
	tmp = (tmp | 0x11);\
	write_cpsr(tmp); };

#define set_world_irq() {\
	unsigned int tmp = read_cpsr();\
	tmp = (tmp & ~(0x1F));\
	tmp = (tmp | 0x12);\
	write_cpsr(tmp); };

#define set_world_svc() {\
	unsigned int tmp = read_cpsr();\
	tmp = (tmp & ~(0x1F));\
	tmp = (tmp | 0x13);\
	write_cpsr(tmp); };

#define set_world_abt() {\
	unsigned int tmp = read_cpsr();\
	tmp = (tmp & ~(0x1F));\
	tmp = (tmp | 0x17);\
	write_cpsr(tmp); };

#define set_world_und() {\
	unsigned int tmp = read_cpsr();\
	tmp = (tmp & ~(0x1F));\
	tmp = (tmp | 0x1B);\
	write_cpsr(tmp); };

#define set_world_sys() {\
	unsigned int tmp = read_cpsr();\
	tmp = (tmp & ~(0x1F));\
	tmp = (tmp | 0x1F);\
	write_cpsr(tmp); };

#define set_world_mon() {\
	unsigned int tmp = read_cpsr();\
	tmp = (tmp & ~(0x1F));\
	tmp = (tmp | 0x16);\
	write_cpsr(tmp); };


#endif
