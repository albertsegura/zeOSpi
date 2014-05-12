/*
 * mm.h - Capçalera del mòdul de gestió de memòria
 */

#ifndef __MM_H__
#define __MM_H__

#include <types.h>
#include <mm_address.h>
#include <sched.h>
 
#define FREE_FRAME 0
#define USED_FRAME 1
/* Bytemap to mark the free physical pages */
extern Byte phys_mem[TOTAL_PH_PAGES];

int init_frames( void );
int alloc_frame( void );
void free_frame( unsigned int frame );

void init_dir_pages(void);
void init_table_pages(void);
void init_empty_pages(void);
void set_coprocessor_reg_MMU(void);

void set_user_pages( struct task_struct *task );

void init_mm();

void enable_icache();
void disable_icache();
void invalidate_icache();

void enable_dcache();
void disable_dcache();
void invalidate_dcache();

void set_ss_pag(sl_page_table_entry *PT, unsigned page,unsigned frame);
void del_ss_pag(sl_page_table_entry *PT, unsigned page);
unsigned int get_frame(sl_page_table_entry *PT, unsigned int page);

void set_vitual_to_phsycial(unsigned int virtual, unsigned ph, char to_current_task);

void allocate_page_dir (struct task_struct *p);
void get_newpb (struct task_struct *p);

/* Temp */
void test_mmu_funct(void);
void monoprocess_init_addr_space(void);

#endif  /* __MM_H__ */
