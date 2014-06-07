#ifndef __TYPES_H__
#define __TYPES_H__

/** System types definition **/
/*****************************/

typedef unsigned char       Byte;
typedef unsigned short int  Word;
typedef unsigned long       DWord;

#define highWord(address)	(Word)(((address) >> 16) & 0xFFFF)
#define lowWord(address)	(Word)((address) & 0xFFFF)
#define midByte(address)	(Byte)(((address) >> 16) & 0xFF)
#define highByte(address)	(Byte)(((address) >> (16 + 8)) & 0xFF)
#define high4Bits(limit)	(Byte)(((limit) >> 16) & 0x0F)



/** Registers: **/
/****************/
#define NULL 0

typedef union
{
  unsigned int entry;
  struct {
    unsigned int accesstype : 2; // 0b01 for second level page table
    unsigned int    		: 1; // should be zero
    unsigned int ns         : 1; // secure(0)/non-secure(1)
    unsigned int 			: 1; // should be zero
    unsigned int domain     : 4;
    unsigned int p          : 1; // ECC enabled, not supported in ARM1176JZF-S
    unsigned int pbase_addr : 22;
  } bits;
} fl_page_table_entry; //page 6-39

typedef union
{
  unsigned int entry;
  struct {
    unsigned int xn         : 1; // execute-never(1), executable(0) (instructions on the memory)
    unsigned int setbit     : 1; // set to 1 for 4KB pages
    unsigned int b          : 1; // bufferable
    unsigned int c          : 1; // cacheable
    unsigned int ap         : 2; // acces permission, use with apb bit. Table 6-1
    unsigned int tex        : 3; // Type extension field. Page 6-14
    unsigned int apx        : 1; // access permission extension
    unsigned int s          : 1; // non-shared(0)/shared(1)
    unsigned int ng         : 1; // global(0)/process-specific(1)
    unsigned int pbase_addr : 20;
  } bits;
} sl_page_table_entry; //page 6-40

typedef union
{
  unsigned int entry;
  struct {
    unsigned int M         : 5;
    unsigned int T         : 1;
    unsigned int dF         : 1;
    unsigned int dI         : 1;
    unsigned int A         : 1;
    unsigned int E         : 1;
    unsigned int 	       : 6; // DNM
    unsigned int GE        : 4;
    unsigned int 		   : 4; // DNM
    unsigned int J	       : 1;
    unsigned int 	       : 2; // DNM
    unsigned int Q         : 1;
    unsigned int V         : 1;
    unsigned int C         : 1;
    unsigned int Z         : 1;
    unsigned int N         : 1;
  } bits;
} cpsr;

typedef union
{
  unsigned int entry;
  struct {
    unsigned int M         : 1;
    unsigned int A         : 1;
    unsigned int C         : 1;
    unsigned int W         : 1;
    unsigned int           : 3; //SB0
    unsigned int B         : 1;
    unsigned int S         : 1;
    unsigned int R         : 1;
    unsigned int F         : 1;
    unsigned int Z         : 1;
    unsigned int I         : 1;
    unsigned int V         : 1;
    unsigned int RR        : 1;
    unsigned int L4        : 1;
    unsigned int DT        : 1;
    unsigned int           : 1; //SB0
    unsigned int IT        : 1;
    unsigned int           : 2; //SB0
    unsigned int FI        : 1;
    unsigned int U         : 1;
    unsigned int XP        : 1;
    unsigned int VE        : 1;
    unsigned int EE        : 1;
    unsigned int           : 2; //SB0
    unsigned int TR        : 1;
    unsigned int FA        : 1;
    unsigned int           : 2; //SB0
  } bits;
} ctrl_reg;


#endif  /* __TYPES_H__ */
