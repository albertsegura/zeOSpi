layout asm
layout regs
target remote localhost:1234
symbol-file system
add-symbol-file user 0x100000
