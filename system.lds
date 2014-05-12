/*
 *  ZeOS
 *  system.lds - Linker Script for ZeOS system image.
 */

ENTRY(main)
SECTIONS
{
  . = 0x10000;
  
  .text.main : { 
    /* reserved to store user code size */
    LONG(0);
    LONG(0);
	LONG(0);
    *(.text.main) 
  }
                                     
  .text : { *(.text) }
  .rodata : { *(.rodata) }
  .data : { *(.data) }
  .bss : { *(.bss) }

  . = ALIGN(4096);              /* task_structs array*/
  .data.task : { *(.data.task) }
  . = ALIGN(16384); 
  .data.mmu_fl_page : { *(.data.mmu_fl_page) }
  . = ALIGN(1024); 
  .data.mmu_sl_page : { *(.data.mmu_sl_page) }
  . = ALIGN(1024); 
  .data.mmu_sl_empty_page : { *(.data.mmu_sl_empty_page) }
  . = ALIGN(4096); 
  .data.mmu_empty_ph_page : { *(.data.mmu_empty_ph_page) }

}
