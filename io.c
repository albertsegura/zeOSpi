/*
 * io.c -
 */

#include <io.h>
#include <uart.h>

void printc(char c) {
	uart_send_byte(c);
}

void printk(char *string) {
  int i;
  for (i = 0; string[i]; i++) printc(string[i]);
}

void printint(unsigned int num) {
	char xifres = 0;
	unsigned int reversed = 0;
	if (num == 0) {
		printc('0');
		return;
	}
	for(;num != 0;num/=10) {
		reversed *= 10;
		reversed += num%10;
		++xifres;
	}
	while (xifres != 0) {
		printc(reversed%10+'0');
		reversed /= 10;
		--xifres;
	}
}

void printhex(unsigned int num) {
	int i;
	printc('0');
	printc('x');
	for (i=28; i>=0; i-=4) {
		unsigned int value = (num>>i)&0xF;
		if (value < 10) printc(value+'0');
		else printc(value+'A'-10);
	}
}
