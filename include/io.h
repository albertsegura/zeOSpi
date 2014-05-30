/*
 * io.h - Definició de l'entrada/sortida en mode sistema
 */

#ifndef __IO_H__
#define __IO_H__

#include <types.h>

void printc(char c);
void printk(char *string);
void printint(unsigned int num);
void printhex(unsigned int num);

#endif  /* __IO_H__ */
