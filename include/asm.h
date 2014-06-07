#ifndef __ASM_H__
#define __ASM_H__

#define ENTRY(name) \
  .globl name; \
  .type name, function; \
  .align 8; \
  name:

#define ENTRY_UA(name) \
  .globl name; \
  .type name, function; \
  name:

#endif  /* __ASM_H__ */
