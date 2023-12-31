# Cross-Compiling makefile for driver: For Kernel 4.4
# Derived from: http://www.opensourceforu.com/2010/12/writing-your-first-linux-driver/
# with some settings from Robert Nelson's BBB kernel build script
#
# use: 
#    make

# if KERNELRELEASE is defined, we've been invoked from the
# kernel build system and can use its language.
ifneq (${KERNELRELEASE},)
	obj-m :=   morsecode.o
	

# Otherwise we were called directly from the command line.
# Invoke the kernel build system.
else
	KERNEL_SOURCE := ~/cmpt433/work/bb-kernel/KERNEL/
	#KERNEL_SOURCE := /usr/src/linux-headers-4.4.12-ti-r31/
	PWD := $(shell pwd)

	# SETUP COMPILER FOR SPECIFIC COMPILER
	# Linux kernel 5.4
	CC=${HOME}/cmpt433/work/bb-kernel/dl/gcc-arm-8.3-2019.03-x86_64-arm-linux-gnueabihf/bin/arm-linux-gnueabihf-
	BUILD=bone13
	CORES=4
	PUBLIC_DRIVER_PWD=~/cmpt433/public/drivers


all: default

default:
	# Trigger kernel build for this module
	${MAKE} -C ${KERNEL_SOURCE} M=${PWD} -j${CORES} ARCH=arm LOCALVERSION=-${BUILD} CROSS_COMPILE=${CC} ${address} ${image} modules
	# copy result to public folder
	cp *.ko ${PUBLIC_DRIVER_PWD}

clean:
	${MAKE} -C ${KERNEL_SOURCE} M=${PWD} clean


endif
