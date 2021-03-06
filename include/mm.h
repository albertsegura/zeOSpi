#ifndef __MM_H__
#define __MM_H__

#include <types.h>
#include <mm_address.h>
#include <sched.h>

#define FREE_FRAME 0
#define USED_FRAME 1

/* Bytemap to mark the free physical pages */
extern Byte phys_mem[TOTAL_PH_PAGES];

int init_frames();
int alloc_frame();
void free_frame( unsigned int frame );

void init_mm();
void init_dir_pages();
void init_table_pages();
void init_empty_pages();
void set_coprocessor_reg_MMU();

void set_user_pages( struct task_struct *task );
void mmu_change_dir (fl_page_table_entry * dir);

void enable_icache();
void disable_icache();

char check_used_page(sl_page_table_entry *pt);
void set_ss_pag(sl_page_table_entry *PT, unsigned page,unsigned frame);
void del_ss_pag(sl_page_table_entry *PT, unsigned page);
unsigned int get_frame(sl_page_table_entry *PT, unsigned int page);

void set_vitual_to_phsycial(unsigned int virtual, unsigned ph, char to_current_task);

/* Clone/heap related functions */
void allocate_page_dir (struct task_struct *p);
void init_pb();
void get_newpb (struct task_struct *p);

#endif  /* __MM_H__ */
