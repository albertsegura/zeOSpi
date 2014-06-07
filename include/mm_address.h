#ifndef __MM_ADDRESS_H__
#define __MM_ADDRESS_H__

#define ENTRY_DIR_PAGES			0

/* MMU size definitions */
#define DIR_BITS				12
#define PAGE_BITS				8
#define OFFSET_BITS				12
#define TOTAL_DIR_ENTRIES		(1<<DIR_BITS)
#define TOTAL_PAGES_ENTRIES		(1<<PAGE_BITS)
#define PAGE_SIZE				(1<<OFFSET_BITS)
#define NUM_DIR_ENTRIES 		2

/* Number of frames/pages utilized by zeOS */
#define TOTAL_PH_PAGES 			1024
#define NUM_PAG_KERNEL 			256
#define NUM_PAG_CODE 			8
#define NUM_PAG_DATA			20

/* General @ definitions*/
#define PAG_LOG_INIT_CODE_P0	(L_USER_START>>12)
#define PAG_LOG_INIT_DATA_P0	(PAG_LOG_INIT_CODE_P0+NUM_PAG_CODE)
// First free pag on a newly created process
#define FIRST_FREE_PAG_P 		(NUM_PAG_KERNEL+NUM_PAG_CODE+NUM_PAG_DATA)
#define USER_FIRST_PAGE			(L_USER_START>>12)

#define INIT_USR_CODE_PAG_D1	0
#define INIT_USR_DATA_PAG_D1	(NUM_PAG_CODE)
#define USER_FIRST_PAGE_D1		0
#define PROC_FIRST_FREE_PAG_D1	(NUM_PAG_CODE+NUM_PAG_DATA)


/* Peripherals/Structure definitions */
#define UART_READ_BUFFER_SIZE 	1024
#define SEM_SIZE 				30
#define HEAPSTART_OLD			(NUM_PAG_KERNEL+NUM_PAG_CODE+NUM_PAG_DATA)
#define USR_P_HEAPSTART 		(NUM_PAG_CODE+NUM_PAG_DATA)

/* Memory distribution */
/***********************/
#define KERNEL_START			0x10000
#define L_USER_START			0x100000
#define PH_USER_START			0x100000
#define USER_SP					L_USER_START+(NUM_PAG_CODE+NUM_PAG_DATA)*0x1000-0x10 //0x11BFF0
#define DIR(x)					(((x)>>(PAGE_BITS+OFFSET_BITS))&(TOTAL_DIR_ENTRIES-1))
#define PAGE(x)					(((x)>>OFFSET_BITS)&(TOTAL_PAGES_ENTRIES-1))
#define	OFFSET(x)				((x)&(PAGE_SIZE-1))
#define PH_PAGE(x)				((x)>>OFFSET_BITS)

#endif

