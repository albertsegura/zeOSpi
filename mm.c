/*
 * mm.c - Memory Management: Paging & segment memory management
 */

#include <types.h>
#include <mm.h>
#include <segment.h>
#include <hardware.h>
#include <sched.h>

Byte phys_mem[TOTAL_PAGES];

/* SEGMENTATION */
/* Memory segements description table */
Descriptor  *gdt = (Descriptor *) GDT_START;
/* Register pointing to the memory segments table */
Register    gdtR;

/* TSS */
TSS         tss;

/* PAGING */
/* Variables containing the page directory and the page table */
  
page_table_entry dir_pages[NR_TASKS][TOTAL_PAGES]
  __attribute__((__section__(".data.task")));

page_table_entry pagusr_table[NR_TASKS][TOTAL_PAGES]
  __attribute__((__section__(".data.task")));


fl_page_table_entry fl_ptable[NR_TASKS][TOTAL_PAGES]
__attribute__((__section__(".data.task")));

sl_page_table_entry sl_ptable[NR_TASKS][TOTAL_PAGES]
__attribute__((__section__(".data.task")));


/***********************************************/
/************** PAGING MANAGEMENT **************/
/***********************************************/

/* Initializes paging for the system address space */
void init_mm()
{
  // Configuration of related Coprocessor registers (secure world)
  set_coprocessor_reg_MMU();

  // Configuration of frist and second level page tables
  init_frames();
  init_sl_ptable();
  init_fl_ptable();

  // Disable&Invalidate Inst cache (secure world)

  //Enable MMU & re-enable Inst cache

}

/* Initializes the page table (kernel pages only) */
void init_sl_ptable(void) {
    int i,j;
    /* reset all entries */
    for (j=0; j< NR_TASKS; j++) {
        for (i=0; i<TOTAL_PAGES; i++) {
            sl_ptable[j][i].entry = 0;
            sl_ptable[j][i].bits.setbit = 1;
        }
        /* Init kernel pages */
        for (i=1; i<NUM_PAG_KERNEL; i++) // Leave the page inaccessible to comply with NULL convention
        {
            // Logical page equal to physical page (frame)
            sl_ptable[j][i].bits.pbase_addr = i;

            /* privileged == rw, user == no access */
            sl_ptable[j][i].bits.ap = 0b01;
            sl_ptable[j][i].bits.apx = 0;

            sl_ptable[j][i].bits.setbit = 1;
            sl_ptable[j][i].bits.xn = 0; // TODO check si es pot separar codi de data

            //sl_ptable[j][i].bits.present = 1;
        }
    }
}

/* Init page table directory */
void init_fl_ptable(void) {
    int i;

    for (i = 0; i< NR_TASKS; i++) {
        fl_ptable[i][ENTRY_DIR_PAGES].entry = 0;
        fl_ptable[i][ENTRY_DIR_PAGES].bits.ptbase_addr = (((unsigned int)&sl_ptable[i]) >> 12);
        fl_ptable[i][ENTRY_DIR_PAGES].bits.accesstype = 0b01; // Necessari?
        fl_ptable[i][ENTRY_DIR_PAGES].bits.ns = 0;
        //fl_ptable[i][ENTRY_DIR_PAGES].bits.user = 1;
        //fl_ptable[i][ENTRY_DIR_PAGES].bits.rw = 1;
        //fl_ptable[i][ENTRY_DIR_PAGES].bits.present = 1;

        //TODO task[i].task.dir_pages_baseAddr = (fl_page_table_entry *)&fl_ptable[i][ENTRY_DIR_PAGES];
    }

}

/* Coprocessor Registers configuration relative to the MMU*/
void set_coprocessor_reg_MMU(void) {
	// TODO llegir registre abans, modificar i escriure
	// 0x0080
	__asm__("MCR P15, 0, r7, c0, c0, 3");
	// Fer en el system
	__asm__("MCR P15, 0, r7, c1, c0, 0");
	// 0x0000
	__asm__("MCR P15, 0, r7, c1, c1, 2");
	// TTB0 0x01
	__asm__("MCR P15, 0, r7, c2, c0, 0");
	// TTB1 0x
	__asm__("MCR P15, 0, r7, c2, c0, 1");
	// TTBC 0x
	__asm__("MCR P15, 0, r7, c2, c0, 2");
	// Domains
	__asm__("MCR P15, 0, r7, c3, c0, 0");
	// Data fault
	__asm__("MCR P15, 0, r7, c5, c0, 0");
	// Inst fault
	__asm__("MCR P15, 0, r7, c5, c0, 1");
	// Fault address register
	__asm__("MCR P15, 0, r7, c6, c0, 0");
	// Inst Fault address register
	__asm__("MCR P15, 0, r7, c6, c0, 2");
	// TLB Operation reg
	__asm__("MCR P15, 0, r7, c8, c5, 0");
	// TLB lockdown reg
	__asm__("MCR P15, 0, r7, c10, c0, 0");
	// Primary region
	__asm__("MCR P15, 0, r7, c10, c2, 0");
	// Normal Memory
	__asm__("MCR P15, 0, r7, c10, c2, 1");
	// FCSE PID
	__asm__("MCR P15, 0, r7, c13, c0, 0");
	// Context ID
	__asm__("MCR P15, 0, r7, c13, c0, 1");
	// Peripheral Port Memory Remap
	__asm__("MCR P15, 0, r7, c15, c2, 4");
	// TLB lockdown access
	__asm__("MCR P15, 0, r7, c15, c4, 2");
	// TLB lockdown access
	__asm__("MCR P15, 0, r7, c15, c5, 2");
	// TLB lockdown access
	__asm__("MCR P15, 0, r7, c15, c6, 2");
	// TLB lockdown access
	__asm__("MCR P15, 0, r7, c15, c7, 2");
}

/* Init page table directory */
void init_dir_pages(void) {
    int i;

    for (i = 0; i< NR_TASKS; i++) {
        dir_pages[i][ENTRY_DIR_PAGES].entry = 0;
        dir_pages[i][ENTRY_DIR_PAGES].bits.pbase_addr = (((unsigned int)&pagusr_table[i]) >> 12);
        dir_pages[i][ENTRY_DIR_PAGES].bits.user = 1;
        dir_pages[i][ENTRY_DIR_PAGES].bits.rw = 1;
        dir_pages[i][ENTRY_DIR_PAGES].bits.present = 1;

        task[i].task.dir_pages_baseAddr = (page_table_entry *)&dir_pages[i][ENTRY_DIR_PAGES];
    }

}

/* Initializes the page table (kernel pages only) */
void init_table_pages()
{
    int i,j;
    /* reset all entries */
    for (j=0; j< NR_TASKS; j++) {
        for (i=0; i<TOTAL_PAGES; i++) {
            pagusr_table[j][i].entry = 0;
        }
    /* Init kernel pages */
        for (i=1; i<NUM_PAG_KERNEL; i++) // Leave the page inaccessible to comply with NULL convention
        {
            // Logical page equal to physical page (frame)
            pagusr_table[j][i].bits.pbase_addr = i;
            pagusr_table[j][i].bits.rw = 1;
            pagusr_table[j][i].bits.present = 1;
        }
    }
}

/* Initialize pages for initial process (user pages) */
void set_user_pages( struct task_struct *task )
{
 int pag; 
 int new_ph_pag;
 page_table_entry * process_PT =  get_PT(task);


  /* CODE */
  for (pag=0;pag<NUM_PAG_CODE;pag++){
	new_ph_pag=alloc_frame();
  	process_PT[PAG_LOG_INIT_CODE_P0+pag].entry = 0;
  	process_PT[PAG_LOG_INIT_CODE_P0+pag].bits.pbase_addr = new_ph_pag;
  	process_PT[PAG_LOG_INIT_CODE_P0+pag].bits.user = 1;
  	process_PT[PAG_LOG_INIT_CODE_P0+pag].bits.present = 1;
  }
  
  /* DATA */ 
  for (pag=0;pag<NUM_PAG_DATA;pag++){
	new_ph_pag=alloc_frame();
  	process_PT[PAG_LOG_INIT_DATA_P0+pag].entry = 0;
  	process_PT[PAG_LOG_INIT_DATA_P0+pag].bits.pbase_addr = new_ph_pag;
  	process_PT[PAG_LOG_INIT_DATA_P0+pag].bits.user = 1;
  	process_PT[PAG_LOG_INIT_DATA_P0+pag].bits.rw = 1;
  	process_PT[PAG_LOG_INIT_DATA_P0+pag].bits.present = 1;
  }
}

/* Writes on CR3 register producing a TLB flush */
void set_cr3(page_table_entry * dir)
{
 	//asm volatile("movl %0,%%cr3": :"r" (dir));
}

/* Macros for reading/writing the CR0 register, where is shown the paging status */
/*#define read_cr0() ({ \
         unsigned int __dummy; \
         __asm__( \
                 "movl %%cr0,%0\n\t" \
                 :"=r" (__dummy)); \ 
         __dummy; \ 
})*/

/*#define write_cr0(x) \
         __asm__("movl %0,%%cr0": :"r" (x));
  */       
/* Enable paging, modifying the CR0 register */
void set_pe_flag()
{
  unsigned int cr0 = 0;//= read_cr0();
  cr0 |= 0x80000000;
  //write_cr0(cr0);
}

/***********************************************/
/************** SEGMENTATION MANAGEMENT ********/
/***********************************************/
void setGdt()
{
  /* Configure TSS base address, that wasn't initialized */
  gdt[KERNEL_TSS>>3].lowBase = lowWord((DWord)&(tss));
  gdt[KERNEL_TSS>>3].midBase  = midByte((DWord)&(tss));
  gdt[KERNEL_TSS>>3].highBase = highByte((DWord)&(tss));

  gdtR.base = (DWord)gdt;
  gdtR.limit = 256 * sizeof(Descriptor);

  set_gdt_reg(&gdtR);
}

/***********************************************/
/************* TSS MANAGEMENT*******************/
/***********************************************/
void setTSS()
{
  tss.PreviousTaskLink   = NULL;
  tss.esp0               = KERNEL_ESP;
  tss.ss0                = __KERNEL_DS;
  tss.esp1               = NULL;
  tss.ss1                = NULL;
  tss.esp2               = NULL;
  tss.ss2                = NULL;
  tss.cr3                = NULL;
  tss.eip                = 0;
  tss.eFlags             = INITIAL_EFLAGS; /* Enable interrupts */
  tss.eax                = NULL;
  tss.ecx                = NULL;
  tss.edx                = NULL;
  tss.ebx                = NULL;
  tss.esp                = USER_ESP;
  tss.ebp                = tss.esp;
  tss.esi                = NULL;
  tss.edi                = NULL;
  tss.es                 = __USER_DS;
  tss.cs                 = __USER_CS;
  tss.ss                 = __USER_DS;
  tss.ds                 = __USER_DS;
  tss.fs                 = NULL;
  tss.gs                 = NULL;
  tss.LDTSegmentSelector = KERNEL_TSS;
  tss.debugTrap          = 0;
  tss.IOMapBaseAddress   = NULL;

  set_task_reg(KERNEL_TSS);
}

 
/* Initializes the ByteMap of free physical pages.
 * The kernel pages are marked as used */
int init_frames( void ) {
    int i;
    /* Mark pages as Free */
    for (i=0; i<TOTAL_PAGES; i++) {
        phys_mem[i] = FREE_FRAME;
    }
    /* Mark kernel pages as Used */
    for (i=0; i<NUM_PAG_KERNEL; i++) {
        phys_mem[i] = USED_FRAME;
    }
    return 0;
}

/* alloc_frame - Search a free physical page (== frame) and mark it as USED_FRAME. 
 * Returns the frame number or -1 if there isn't any frame available. */
int alloc_frame( void )
{
    int i;
    for (i=NUM_PAG_KERNEL; i<TOTAL_PAGES;) {
        if (phys_mem[i] == FREE_FRAME) {
            phys_mem[i] = USED_FRAME;
            return i;
        }
        i += 2; /* NOTE: There will be holes! This is intended. 
			DO NOT MODIFY! */
    }

    return -1;
}

void free_user_pages( struct task_struct *task )
{
 int pag;
 page_table_entry * process_PT =  get_PT(task);
    /* DATA */
 for (pag=0;pag<NUM_PAG_DATA;pag++){
	 free_frame(process_PT[PAG_LOG_INIT_DATA_P0+pag].bits.pbase_addr);
         process_PT[PAG_LOG_INIT_DATA_P0+pag].entry = 0;
 }
}


/* free_frame - Mark as FREE_FRAME the frame  'frame'.*/
void free_frame( unsigned int frame )
{
    /* You must insert code here */
    if ((frame>NUM_PAG_KERNEL)&&(frame<TOTAL_PAGES))
      phys_mem[frame]=FREE_FRAME;
}

/* set_ss_pag - Associates logical page 'page' with physical page 'frame' */
void set_ss_pag(page_table_entry *PT, unsigned page,unsigned frame)
{
	PT[page].entry=0;
	PT[page].bits.pbase_addr=frame;
	PT[page].bits.user=1;
	PT[page].bits.rw=1;
	PT[page].bits.present=1;

}

/* del_ss_pag - Removes mapping from logical page 'logical_page' */
void del_ss_pag(page_table_entry *PT, unsigned logical_page)
{
  PT[logical_page].entry=0;
}

/* get_frame - Returns the physical frame associated to page 'logical_page' */
unsigned int get_frame (page_table_entry *PT, unsigned int logical_page){
     return PT[logical_page].bits.pbase_addr; 
}
