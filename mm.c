/*
 * mm.c - Memory Management: Paging & segment memory management
 */

#include <types.h>
#include <mm.h>
#include <segment.h>
#include <hardware.h>
#include <sched.h>
#include <utils.h>

Byte phys_mem[TOTAL_PH_PAGES];


/* PAGING */
/* Variables containing the page directory and the page table */

fl_page_table_entry fl_ptable[NR_TASKS][TOTAL_DIR_ENTRIES]
__attribute__((__section__(".data.mmu_fl_page")));

sl_page_table_entry sl_ptable[NR_TASKS][NUM_DIR_ENTRIES][TOTAL_PAGES_ENTRIES]
__attribute__((__section__(".data.mmu_sl_page")));

sl_page_table_entry empty_sl_ptable[TOTAL_PAGES_ENTRIES]
__attribute__((__section__(".data.mmu_sl_empty_page")));

Byte empty_ph_page[4096]
__attribute__((__section__(".data.mmu_empty_ph_page")));

/* Counters for the references to a same directory entry. */
Byte assigned_base_dir[NR_TASKS];

/***********************************************/
/************** PAGING MANAGEMENT **************/
/***********************************************/

/* Initializes paging for the system address space */
void init_mm() {
	unsigned int mmu_en=0;
	/* 1. Program all relevant CP15 registers of the corresponding world. */
	set_coprocessor_reg_MMU();

	/* 2. Program first-level and second-level descriptor page tables as required. */
	init_frames();
	init_empty_pages();
	init_table_pages();
	init_dir_pages();

	/* 3. Disable and invalidate the Instruction Cache for the corresponding world. You can then
	 * re-enable the Instruction Cache when you enable the MMU. */
	//invalidate_icache();
	//disable_icache();

	__asm__ __volatile__ (
		"MCR P15, 0,  %0,  c7, c7, 0;" // invalidate both caches
		"MCR P15, 0,  %0,  c8, c7, 0;" // invalidate tlb
		: "+r"(mmu_en)
	);
    asm volatile ("mcr p15, 0, %0, c7, c5,  4" :: "r" (0) : "memory");
    asm volatile ("mcr p15, 0, %0, c7, c6,  0" :: "r" (0) : "memory");

	/* 4. Enable the MMU by setting bit 0 in the CP15 Control Register in the corresponding world. */
	ctrl_reg creg;
	__asm__ __volatile__ ("MRC P15, 0,  %0,  c1, c0, 0;" : "=r"(creg));
	creg.bits.XP = 1;
	creg.bits.M = 1;
	__asm__ __volatile__ ("MCR P15, 0,  %0,  c1, c0, 0;" : : "r"(creg));

	enable_icache();

}

void set_empty_page(sl_page_table_entry * empty_page) {
	empty_page->entry = 0;
	empty_page->bits.pbase_addr = PH_PAGE(((int)&empty_ph_page[0]));

    empty_page->bits.xn = 1;
    empty_page->bits.setbit = 1;
    empty_page->bits.b = 0;
    empty_page->bits.c = 0;

    /* privileged == rw, user == rw */
    empty_page->bits.ap = 0b11;
    empty_page->bits.apx = 0;
    empty_page->bits.tex = 0;
    empty_page->bits.s = 1;
    empty_page->bits.ng = 1;
}

void init_empty_pages(void) {
	int i;
    /* Set empty pages to known and controlled memory space */
    for (i=0; i<TOTAL_PAGES_ENTRIES; i++) {
    	set_empty_page(&(empty_sl_ptable[i]));
    }
    for (i=0; i<4096; i+=8) {
    	empty_ph_page[i]	= 0x67;
    	empty_ph_page[i+1] 	= 0x45;
    	empty_ph_page[i+2]	= 0x23;
    	empty_ph_page[i+3]	= 0x01;
    	empty_ph_page[i+4]	= 0xef;
    	empty_ph_page[i+5]	= 0xcd;
    	empty_ph_page[i+6]	= 0xab;
    	empty_ph_page[i+7]	= 0x89;
    }

}

char check_used_page(sl_page_table_entry *pt) {
	return (pt->entry != empty_sl_ptable[0].entry);
}

/* Initializes the page table (kernel pages only) */
void init_table_pages(void) {
    int i,j,k;
    /* reset all entries */
    for (i=0; i< NR_TASKS; i++) {
        for (j=0; j<NUM_DIR_ENTRIES; j++) {
        	for (k=0; k<TOTAL_PAGES_ENTRIES; k++) {
				set_empty_page(&(sl_ptable[i][j][k]));
        	}
        }
        /* Init kernel pages */
        for (k=0; k<NUM_PAG_KERNEL; k++) {
            // Logical page equal to physical page (frame)
            sl_ptable[i][0][k].bits.pbase_addr = k;

            sl_ptable[i][0][k].bits.xn = 0; // TODO check si es pot separar codi de data
            sl_ptable[i][0][k].bits.setbit = 1;
            sl_ptable[i][0][k].bits.b = 0;
            sl_ptable[i][0][k].bits.c = 0;

            /* privileged == rw, user == no access */
            sl_ptable[i][0][k].bits.ap = 0b01;
            sl_ptable[i][0][k].bits.apx = 0;
            sl_ptable[i][0][k].bits.tex = 0;
            sl_ptable[i][0][k].bits.s = 1;
            sl_ptable[i][0][k].bits.ng = 1; // global vs nonglobal (process specific)
        }
    }
}

/* Init page table directory */
void init_dir_pages(void) {
    int i, j;

    for (i = 0; i < NR_TASKS; i++) {
    	assigned_base_dir[i] = 0;
    	//task[i].task.dir_pages_baseAddr = (fl_page_table_entry *)&fl_ptable[i][0];

    	for (j=0; j < NUM_DIR_ENTRIES; j++) {
			fl_ptable[i][j].entry = 0;
			fl_ptable[i][j].bits.accesstype = 0b01;
			fl_ptable[i][j].bits.ns = 0;
			fl_ptable[i][j].bits.domain = 0; // TODO test value
			fl_ptable[i][j].bits.p = 0;
			fl_ptable[i][j].bits.pbase_addr = (((unsigned int)&sl_ptable[i][j][ENTRY_DIR_PAGES]) >> 10);
    	}

    	/* Set unused entries to known and controlled memory space */
    	for (j=NUM_DIR_ENTRIES; j < TOTAL_DIR_ENTRIES; j++) {
			fl_ptable[i][j].entry = 0;
			fl_ptable[i][j].bits.accesstype = 0b01;
			fl_ptable[i][j].bits.ns = 0;
			fl_ptable[i][j].bits.domain = 0; // TODO test value
			fl_ptable[i][j].bits.p = 0;
			fl_ptable[i][j].bits.pbase_addr = (((unsigned int)&empty_sl_ptable[ENTRY_DIR_PAGES]) >> 10);
    	}
    }
}

void test_mmu_funct(void) {
	unsigned int mmu_en = 0;
	unsigned int * pt;

	pt = (unsigned int *) 0xF0000;
	*(pt) = 0xdeadbeef;
	pt = (unsigned int *)0xF1000;
	*(pt) = 0x0f0f0f0f;

	sl_ptable[0][0][0].bits.pbase_addr = 241;
	sl_ptable[0][0][241].bits.pbase_addr = 240;
    //asm volatile ("mcr p15, 0, %0, c7, c5,  4" :: "r" (0) : "memory");
    //asm volatile ("mcr p15, 0, %0, c7, c6,  0" :: "r" (0) : "memory");
	__asm__ __volatile__ (
			"MCR P15, 0,  %0,  c7, c7, 0;" // invalidate both caches
			"MCR P15, 0,  %0,  c8, c7, 0;" // invalidate tlb
			: "+r"(mmu_en)
	);
}

void test_mmu_tlb_status(void) {
	unsigned int volatile aux;
	unsigned int volatile * pt;

	aux = 0x30;
	//dissable PD0/1 Page table walks
	__asm__ __volatile__ (
		"MCR P15, 0,  %0,  c2, c0, 2;"
		: "+r"(aux)
	);

	pt = (unsigned int *) 0xE0000;
	*(pt) = 0xdeadbeef;
	sl_ptable[0][0][224].bits.pbase_addr = 0;
	aux = get_value_from(0xE0000);

	// Test invalidate caches

	if (aux == 0xdeadbeef) {
		printk("TLB OFF\n");
	}
	else printk("TLB ON\n");
	printhex(aux);

	while(1);
}

/* Coprocessor Registers configuration relative to the MMU*/
void set_coprocessor_reg_MMU(void) {
	unsigned int ttb = (unsigned int)&fl_ptable[1][ENTRY_DIR_PAGES];
	__asm__ __volatile__ (
		"MCR P15, 0,  %0,  c1, c1, 2;" 	// Non-secure address control
		"MCR P15, 0,  %1,  c2, c0, 0;"	// TTB0
		"MCR P15, 0,  %2,  c2, c0, 1;"	// TTB1
		"MCR P15, 0,  %3,  c2, c0, 2;"	// TTBC

		"MCR P15, 0,  %4,  c3, c0, 0;"	// Domains TODO domains?
		//"MCR P15, 0,  %5,  c5, c0, 0;"	// Data fault, Produce status of error
		//"MCR P15, 0,  %6,  c5, c0, 1;"	// Inst fault, Produce status of error
		//"MCR P15, 0,  %7,  c6, c0, 0;"	// Fault address register, Produce status of error

		//"MCR P15, 0,  %8,  c6, c0, 2;"	// Inst Fault address register, Produce status of error
		//"MCR P15, 0,  %9,  c8, c5, 0;"	// TLB instruction reg, Used to invalidate TLB
		"MCR P15, 0,  %5, c10, c0, 0;"	// TLB lockdown reg // O for the moment
		"MCR P15, 0,  %6, c10, c2, 0;"	// Primary region
		"MCR P15, 0,  %7, c10, c2, 1;"	// Normal Memory

		"MCR P15, 0,  %8, c13, c0, 0;"	// FCSE PID. Deprecated in favor of register Context ID
		"MCR P15, 0,  %9, c13, c0, 1;"	// Context ID
		"MCR P15, 0, %10, c15, c2, 4;"	// Peripheral Port Memory Remap
		"MCR P15, 5, %11, c15, c4, 2;"	// TLB lockdown access

		"MCR P15, 5, %12, c15, c5, 2;"	// TLB lockdown access
		"MCR P15, 5, %13, c15, c6, 2;" 	// TLB lockdown access
		"MCR P15, 5, %14, c15, c7, 2;"	// TLB lockdown access
		: /* no output */				// TODO valor anterior de TTBC=0
		: "r"(0), "r"(ttb), "r"(ttb), "r"(0x30), \
		  "r"(0xFFFFFFFF), "r"(0), "r"(0x98AA4), "r"(0x44E048E0), \
		  "r"(0), "r"(0), "r"(0), "r"(0), \
		  "r"(0), "r"(0), "r"(0)
	);
}

/*
 * Cache Management page 3-8
 * c7 Cache operations
 *
 * Invalidate inst cache 0 c5 0		<-
 * Flush prefetch buffer 0 c5 4  	<-
 * Invalidate data cache 0 c6 0  	x
 * invalidate both 0 c7 0		 	x
 * Clean data cache 0 c10 0			x
 * Clean and invalidate data cache 0 c14 0	x
 *
 */

void enable_icache() {
	unsigned int creg=0;
	__asm__ __volatile__ (
		"MRC P15, 0,  %0,  c1, c0, 0;"
		"ORR %0, %0, #4096;"
		"MCR P15, 0,  %0,  c1, c0, 0;"
		: "+r"(creg)
	);
}

void disable_icache() {
	unsigned int creg=0;
	__asm__ __volatile__ (
		"MRC P15, 0,  %0,  c1, c0, 0;"
		"BIC %0, %0, #4096;"
		"MCR P15, 0,  %0,  c1, c0, 0;"
		: "+r"(creg)
	);
}

void invalidate_icache() {
	unsigned int cpsrreg=0;
	// TODO pensar si esquema de pagina 3.75 es necessari
	// depen si les caches es poden tenir habilitades/deshabilitades en cada món

	__asm__ __volatile__ (
		"MRS %0, cpsr_all;"
		"BIC %0, %0, #192;"
		"MCR P15, 0,  %0,  c7, c5, 0;"
		"ORR %0, %0, #192;"
		"MSR cpsr_all, %0;"
		: "+r"(cpsrreg)
	);

}

void enable_dcache() {
	unsigned int creg=0;
	__asm__ __volatile__ (
		"MRC P15, 0,  %0,  c1, c0, 0;"
		"ORR %0, %0, #4;"
		"MCR P15, 0,  %0,  c1, c0, 0;"
		: "+r"(creg)
	);
}

void disable_dcache() {
	unsigned int creg=0;
	__asm__ __volatile__ (
		"MRC P15, 0,  %0,  c1, c0, 0;"
		"BIC %0, %0, #4;"
		"MCR P15, 0,  %0,  c1, c0, 0;"
		: "+r"(creg)
	);
}

void invalidate_dcache() {
	// TODO Complete if required
	__asm__ __volatile__ (
		"MCR P15, 0,  %0,  c7, c6, 0;"
		: /* no output */
		: "r"(0)
	);

}

void monoprocess_init_addr_space(void) {
	set_user_pages(&task[0].task);
	// TODO invalidate tlb
	/*
    asm volatile ("mcr p15, 0, %0, c7, c5,  4" :: "r" (0) : "memory");
    asm volatile ("mcr p15, 0, %0, c7, c6,  0" :: "r" (0) : "memory");
	__asm__ __volatile__ (
			"MCR P15, 0,  %0,  c7, c7, 0;" // invalidate both caches
			"MCR P15, 0,  %0,  c8, c7, 0;" // invalidate tlb
			:
			: "r" (0)
	);*/
}

/* Initialize pages for initial process (user pages) */
void set_user_pages( struct task_struct *task ) {
	int pag;
	int new_ph_pag;
	sl_page_table_entry * process_PT =  get_PT(task,1);

	/* CODE */
	for (pag=0;pag<NUM_PAG_CODE;pag++){
		new_ph_pag=alloc_frame();
		process_PT[pag].entry = 0;
		process_PT[pag].bits.pbase_addr = new_ph_pag;

		process_PT[pag].bits.xn = 0;
		process_PT[pag].bits.setbit = 1;
		process_PT[pag].bits.b = 0;
		process_PT[pag].bits.c = 0;

		/* privileged == rw, user == r */
		process_PT[pag].bits.ap = 0b10;
		process_PT[pag].bits.apx = 0;
		process_PT[pag].bits.tex = 0;
		process_PT[pag].bits.s = 1;
		process_PT[pag].bits.ng = 1;
	}

	/* DATA */
	for (pag=NUM_PAG_CODE;pag<NUM_PAG_DATA+NUM_PAG_CODE;pag++){
		new_ph_pag=alloc_frame();
		process_PT[pag].entry = 0;
		process_PT[pag].bits.pbase_addr = new_ph_pag;

		process_PT[pag].bits.xn = 1; // Not executable
		process_PT[pag].bits.setbit = 1;
		process_PT[pag].bits.b = 0;
		process_PT[pag].bits.c = 0;

		/* privileged == rw, user == rw */
		process_PT[pag].bits.ap = 0b11;
		process_PT[pag].bits.apx = 0;
		process_PT[pag].bits.tex = 0;
		process_PT[pag].bits.s = 1;
		process_PT[pag].bits.ng = 1;
	}
}

/* Sets the page of the virtual address to the page of the ph address of the current
 * task if "to_current_task==1" of all of the tasks if "to_current_task=0".
 *  WARNING: Virtual address has to be lower than 0x200000.
 *  Reason: not enough directory entries implemented  */
void set_vitual_to_phsycial(unsigned int virtual, unsigned ph, char to_current_task) {
	int i;
	unsigned int dir = ((virtual>>20)&0x1);
	unsigned int page = ((virtual>>12)&0xFF);
	if (to_current_task) {
		// TODO Only if required
	}
	else {
		for (i=0; i< NR_TASKS; i++) {
			sl_ptable[i][dir][page].bits.pbase_addr = (ph>>12);
		}
	}
}

void mmu_change_dir (fl_page_table_entry * dir) {
	__asm__ __volatile__ (
			"mcr P15, 0,  %0,  c2, c0, 0;"	// TTB0
			"mcr P15, 0,  %0,  c2, c0, 1;"	// TTB1
			: /* no output */
			: "r"(dir)
	);
	// TODO invalidate tlb
}


/***********************************************/
/************** FRAMES MANAGEMENT **************/
/***********************************************/

/* Initializes the ByteMap of free physical pages.
 * The kernel pages are marked as used */
int init_frames( void ) {
    int i;
    /* Mark pages as Free */
    for (i=0; i<TOTAL_PH_PAGES; i++) {
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
int alloc_frame( void ) {
    int i;
    for (i=NUM_PAG_KERNEL; i<TOTAL_PH_PAGES;) {
        if (phys_mem[i] == FREE_FRAME) {
            phys_mem[i] = USED_FRAME;
            return i;
        }
        i += 2; /* NOTE: There will be holes! This is intended. 
			DO NOT MODIFY! */
    }

    return -1;
}

void free_user_pages( struct task_struct *task ) {
	int pag, dir_entry;
	sl_page_table_entry * process_PT;
	/* DATA */
	for (dir_entry=1;dir_entry<NUM_DIR_ENTRIES;dir_entry++){
		process_PT =  get_PT(task,dir_entry);
		for (pag=0;pag<256;pag++){
			free_frame(process_PT[pag].bits.pbase_addr);
			process_PT[pag].entry = 0;
		}
	}
}


/* free_frame - Mark as FREE_FRAME the frame  'frame'.*/
void free_frame( unsigned int frame ) {
    /* You must insert code here */
	if ((frame>NUM_PAG_KERNEL)&&(frame<TOTAL_PH_PAGES))
		phys_mem[frame]=FREE_FRAME;
}

/* set_ss_pag - Associates logical page 'page' with physical page 'frame' */
void set_ss_pag(sl_page_table_entry *PT, unsigned page,unsigned frame) {
	PT[page].entry=0;
	PT[page].bits.pbase_addr=frame;
	PT[page].bits.xn = 1; // Not executable
	PT[page].bits.setbit = 1;
	PT[page].bits.b = 0;
	PT[page].bits.c = 0;

    /* privileged == rw, user == rw */
	PT[page].bits.ap = 0b11;
	PT[page].bits.apx = 0;
	PT[page].bits.tex = 0;
  	PT[page].bits.s = 1;
  	PT[page].bits.ng = 1;
}

/* del_ss_pag - Removes mapping from logical page 'logical_page' */
void del_ss_pag(sl_page_table_entry *PT, unsigned logical_page) {
  PT[logical_page].entry=empty_sl_ptable[0].entry;
}

/* get_frame - Returns the physical frame associated to page 'logical_page' */
unsigned int get_frame (sl_page_table_entry *PT, unsigned int logical_page) {
     return PT[logical_page].bits.pbase_addr; 
}

/* allocate_page_dir - Assignates a dir page to a task_struct and initializes its reference counter */
void allocate_page_dir (struct task_struct *p) {
	int i;
	char found = 0;
	for (i=0; i<NR_TASKS && !found; ++i) {
		found = (assigned_base_dir[i] == 0);
	}
	--i;
	p->dir_pages_baseAddr = (fl_page_table_entry *)&fl_ptable[i][ENTRY_DIR_PAGES];
	p->dir_count = &assigned_base_dir[i];
	assigned_base_dir[i] = 1;
}



