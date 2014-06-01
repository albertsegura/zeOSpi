##################################
############## zeOS ##############
##################################
############ Makefile ############
##################################

LOCTOOL = ../tools/arm-bcm2708/gcc-linaro-arm-linux-gnueabihf-raspbian/bin

HOSTCFLAGS = -Wall -Wstrict-prototypes -g
HOSTCC 	= gcc
CC      = $(LOCTOOL)/arm-linux-gnueabihf-gcc
AS      = $(LOCTOOL)/arm-linux-gnueabihf-as
LD      = $(LOCTOOL)/arm-linux-gnueabihf-ld
OBJCOPY = $(LOCTOOL)/arm-linux-gnueabihf-objcopy -O binary -R .note -R .comment -S

INCLUDEDIR = include

# Define here flags to compile the tests if needed
JP = 
CFLAGS = -mtune=arm1176jzf-s -march=armv6 -O2 -ggdb $(JP) -ffreestanding -fno-stack-protector -Wall -I$(INCLUDEDIR)
ASMFLAGS = -I$(INCLUDEDIR)
SYSLDFLAGS = -T system.lds
USRLDFLAGS = -T user.lds
LINKFLAGS = -g 

SYSOBJ = interrupt.o entry.o sys_call_table.o interrupt_asm.o io.o uart.o gpio.o timer.o sched.o sys.o mm.o devices.o utils.o hardware.o errno.o

#add to USROBJ the object files required to complete the user program
USROBJ = libc.o perror.o errno.o

all: zeos.bin kernel.img



kernel.img: zeos.bin
	cp zeos.bin kernel.img
	
zeos.bin: system build user
	$(OBJCOPY) system system.out
	$(OBJCOPY) user user.out
	./build system.out user.out > zeos.bin

build: build.c
	$(HOSTCC) $(HOSTCFLAGS) -o $@ $<

entry.s: entry.S $(INCLUDEDIR)/asm.h $(INCLUDEDIR)/segment.h
	$(CPP) $(ASMFLAGS) -o $@ $<

sys_call_table.s: sys_call_table.S $(INCLUDEDIR)/asm.h $(INCLUDEDIR)/segment.h
	$(CPP) $(ASMFLAGS) -o $@ $<

interrupt_asm.s: interrupt_asm.S $(INCLUDEDIR)/asm.h $(INCLUDEDIR)/segment.h
	$(CPP) $(ASMFLAGS) -o $@ $<


user.o:user.c $(INCLUDEDIR)/libc.h

interrupt.o:interrupt.c $(INCLUDEDIR)/interrupt.h $(INCLUDEDIR)/segment.h $(INCLUDEDIR)/types.h

io.o:io.c $(INCLUDEDIR)/io.h

uart.o:uart.c $(INCLUDEDIR)/uart.h

gpio.o:gpio.c $(INCLUDEDIR)/gpio.h

timer.o:timer.c $(INCLUDEDIR)/timer.h

sched.o:sched.c $(INCLUDEDIR)/sched.h

libc.o:libc.c $(INCLUDEDIR)/libc.h

perror.o:perror.c $(INCLUDEDIR)/perror.h $(INCLUDEDIR)/libc.h

errno.o:errno.c $(INCLUDEDIR)/errno.h 

mm.o:mm.c $(INCLUDEDIR)/types.h $(INCLUDEDIR)/mm.h

sys.o:sys.c $(INCLUDEDIR)/devices.h 

utils.o:utils.c $(INCLUDEDIR)/utils.h

system.o:system.c $(INCLUDEDIR)/hardware.h system.lds $(SYSOBJ) $(INCLUDEDIR)/segment.h $(INCLUDEDIR)/types.h $(INCLUDEDIR)/interrupt.h \
		$(INCLUDEDIR)/system.h $(INCLUDEDIR)/sched.h $(INCLUDEDIR)/mm.h $(INCLUDEDIR)/io.h $(INCLUDEDIR)/uart.h $(INCLUDEDIR)/gpio.h \
		$(INCLUDEDIR)/timer.h $(INCLUDEDIR)/mm_address.h $(INCLUDEDIR)/errno.h 



system: system.o system.lds $(SYSOBJ)
	$(LD) $< $(SYSOBJ) $(LINKFLAGS) $(SYSLDFLAGS) -o $@

user: user.o user.lds $(USROBJ) 
	$(LD) $< $(USROBJ) $(LINKFLAGS) $(USRLDFLAGS) -o $@



clean:
	rm -f *.o *.s bochsout.txt parport.out system.out system zeos.bin user user.out *~ build kernel.img 

debug: zeos.bin
	qemu-system-arm -s -S -kernel zeos.bin -cpu arm1176 -m 256 -M versatilepb -no-reboot &
	$(LOCTOOL)/arm-linux-gnueabihf-gdb -tui

qemu: zeos.bin
	qemu-system-arm -s -S -kernel zeos.bin -cpu arm1176 -m 256 -M versatilepb -no-reboot &

gdb: zeos.bin
	$(LOCTOOL)/arm-linux-gnueabihf-gdb -tui

