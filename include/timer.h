#ifndef __TIMER_H__
#define __TIMER_H__

#include <types.h>
#include <utils.h>


#define TIMER_BASE_PH		0x2000B000
#define TIMER_BASE			0xF3000	/* ph 0x2000B000 */

#define TIMER_LOAD			(TIMER_BASE+0x400)
#define TIMER_VALUE			(TIMER_BASE+0x404)
#define TIMER_CNTL			(TIMER_BASE+0x408)
#define TIMER_IRQ_CLR		(TIMER_BASE+0x40C)
#define TIMER_RAQ_IRQ		(TIMER_BASE+0x410)
#define TIMER_MSKD_IRQ		(TIMER_BASE+0x404)
#define TIMER_RELOAD		(TIMER_BASE+0x408)
#define TIMER_PREDIVIDER	(TIMER_BASE+0x40C)
#define TIMER_FREE_RUNNING	(TIMER_BASE+0x420)

void init_timer();
void timer_clear_irq();
void timer_set_initial_time(unsigned int time);

void clock_increase();
unsigned int clock_get_time();
void clock_set_time(unsigned long time);


#endif  /* __TIMER_H__ */
