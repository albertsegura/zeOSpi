/*
 * timer.c -
 */

#include <timer.h>
#include <mm.h>

volatile unsigned int clock_time;

void init_timer() {
	set_vitual_to_phsycial(TIMER_BASE,TIMER_BASE_PH,0);

	clock_time = 0;
	timer_set_initial_time(1000); // 1ms == 1 int
	set_address_to(TIMER_CNTL, 0xF900A2);
}

void timer_clear_irq() {
	set_address_to(TIMER_IRQ_CLR, -1);
}

void timer_set_initial_time(unsigned int time) {
	set_address_to(TIMER_LOAD, time);
}

///////////// Clock /////////////

void clock_increase() {
	clock_time++;
}

unsigned int clock_get_time() {
	return clock_time;
}

void clock_set_time(unsigned long time) {
	clock_time = time;
}
