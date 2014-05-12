/*
 * segment.h - Constants de segment per a les entrades de la GDT
 */

#ifndef __SEGMENT_H__
#define __SEGMENT_H__

/* Segment Selectors */
/*********************/

#define __KERNEL_CS     0x10  /* 2 */
#define __KERNEL_DS     0x18  /* 3 */

#define __USER_CS       0x23  /* 4 */
#define __USER_DS       0x2B  /* 5 */

#endif  /* __SEGMENT_H__ */
