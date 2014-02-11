################################
############ zeOS ##############
################################
########## Makefile ############
################################

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
CFLAGS = -mtune=arm1176jzf-s -march=armv6 -O2 -g $(JP) -ffreestanding -fno-stack-protector -Wall -I$(INCLUDEDIR)
ASMFLAGS = -I$(INCLUDEDIR)
SYSLDFLAGS = -T system.lds
USRLDFLAGS = -T user.lds
LINKFLAGS = -g 

SYSOBJ = interrupt.o entry.o sys_call_table.o sched.o sys.o mm.o utils.o hardware.o

LIBZEOS = #-L . -l zeos

#add to USROBJ the object files required to complete the user program
USROBJ = #libc.o perror.o errno.o# libjp.a

all: zeos.bin



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



user.o:user.c 

interrupt.o:interrupt.c $(INCLUDEDIR)/interrupt.h $(INCLUDEDIR)/segment.h $(INCLUDEDIR)/types.h

sched.o:sched.c $(INCLUDEDIR)/sched.h

mm.o:mm.c $(INCLUDEDIR)/types.h $(INCLUDEDIR)/mm.h

sys.o:sys.c

utils.o:utils.c $(INCLUDEDIR)/utils.h



system.o:system.c $(INCLUDEDIR)/hardware.h system.lds $(SYSOBJ) $(INCLUDEDIR)/segment.h $(INCLUDEDIR)/types.h $(INCLUDEDIR)/interrupt.h $(INCLUDEDIR)/system.h $(INCLUDEDIR)/sched.h $(INCLUDEDIR)/mm.h $(INCLUDEDIR)/mm_address.h 



system: system.o system.lds $(SYSOBJ)
	$(LD) $(LINKFLAGS) $(SYSLDFLAGS) -o $@ $< $(SYSOBJ) $(LIBZEOS) 

user: user.o user.lds $(USROBJ) 
	$(LD) $(LINKFLAGS) $(USRLDFLAGS) -o $@ $< $(USROBJ)



clean:
	rm -f *.o *.s bochsout.txt parport.out system.out system zeos.bin user user.out *~ build 

debug: zeos.bin
	qemu-system-arm -s -S -kernel zeos.bin -cpu arm1176 -m 256 -M versatilepb -no-reboot &
	$(LOCTOOL)/arm-linux-gnueabihf-gdb

qemu: zeos.bin
	qemu-system-arm -s -S -kernel zeos.bin -cpu arm1176 -m 256 -M versatilepb -no-reboot &

gdb: zeos.bin
	$(LOCTOOL)/arm-linux-gnueabihf-gdb

raspbuild: zeos.bin


