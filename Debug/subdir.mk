################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../build.c \
../devices.c \
../errno.c \
../gpio.c \
../hardware.c \
../interrupt.c \
../io.c \
../libc.c \
../mm.c \
../perror.c \
../sched.c \
../sys.c \
../system.c \
../timer.c \
../uart.c \
../user.c \
../utils.c 

S_UPPER_SRCS += \
../entry.S \
../interrupt_asm.S \
../sys_call_table.S 

OBJS += \
./build.o \
./devices.o \
./entry.o \
./errno.o \
./gpio.o \
./hardware.o \
./interrupt.o \
./interrupt_asm.o \
./io.o \
./libc.o \
./mm.o \
./perror.o \
./sched.o \
./sys.o \
./sys_call_table.o \
./system.o \
./timer.o \
./uart.o \
./user.o \
./utils.o 

C_DEPS += \
./build.d \
./devices.d \
./errno.d \
./gpio.d \
./hardware.d \
./interrupt.d \
./io.d \
./libc.d \
./mm.d \
./perror.d \
./sched.d \
./sys.d \
./system.d \
./timer.d \
./uart.d \
./user.d \
./utils.d 


# Each subdirectory must supply rules for building sources it contributes
%.o: ../%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross GCC Compiler'
	-mtune=arm1176jzf-s -march=armv6 -O2gcc -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

%.o: ../%.S
	@echo 'Building file: $<'
	@echo 'Invoking: Cross GCC Assembler'
	-mtune=arm1176jzf-s -march=armv6 -O2as  -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


