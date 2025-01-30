# Cross-Compiling Makefile for Linux Kernel Module: Morse Code Driver
# Compatible with Kernel Version 4.4 and above
#
# Derived from:
# - http://www.opensourceforu.com/2010/12/writing-your-first-linux-driver/
# - Robert Nelson's BeagleBone Black (BBB) kernel build script
#
# Usage:
#   make                # Build the kernel module
#   make clean          # Clean build artifacts

# =============================================================================
# User Configuration Section
# =============================================================================

# ------------------------------
# Path to the Linux Kernel Source
# ------------------------------
# Users must set the KERNEL_SRC variable to point to their kernel source directory.
# This directory should contain the 'Makefile' and other kernel build files.
# Example:
#   export KERNEL_SRC=/usr/src/linux-headers-$(uname -r)/
#
# Uncomment and set the path below or set it as an environment variable before running make.
# KERNEL_SRC := /path/to/your/kernel/source/

# ------------------------------
# Cross-Compiler Prefix
# ------------------------------
# For cross-compiling, set the CROSS_COMPILE variable to the toolchain prefix.
# Example for ARM architecture:
#   export CROSS_COMPILE=/usr/bin/arm-linux-gnueabihf-
#
# Uncomment and set the prefix below or set it as an environment variable before running make.
# CROSS_COMPILE := /path/to/your/cross-compiler-prefix-

# ------------------------------
# Number of Parallel Build Jobs
# ------------------------------
# Set CORES to the number of CPU cores available for parallel compilation.
# Defaults to the number of processing units available on the system.
# Example:
#   export CORES=4
#
# Uncomment and set the number below or set it as an environment variable before running make.
# CORES := 4

# ------------------------------
# Build Configuration
# ------------------------------
# BUILD_ID can be used to differentiate builds, especially when deploying to different environments.
# Example:
#   export BUILD_ID=bone13
#
# Uncomment and set the identifier below or set it as an environment variable before running make.
# BUILD_ID := bone13

# ------------------------------
# Output Directory for Compiled Modules
# ------------------------------
# PUBLIC_DRIVER_DIR specifies where the compiled .ko files will be copied after a successful build.
# Example:
#   export PUBLIC_DRIVER_DIR=~/public/drivers
#
# Uncomment and set the path below or set it as an environment variable before running make.
# PUBLIC_DRIVER_DIR := /path/to/public/drivers/

# =============================================================================
# End of User Configuration Section
# =============================================================================

# =============================================================================
# Automatic Configuration Section
# =============================================================================

# Determine if the Makefile is being called from the kernel build system
ifneq ($(KERNELRELEASE),)

	# If KERNELRELEASE is defined, we are being invoked from the kernel build system
	obj-m := morsecode.o

else

	# Ensure that essential variables are set
	ifndef KERNEL_SRC
		$(error "KERNEL_SRC is not set. Please set KERNEL_SRC to your kernel source directory.")
	endif

	ifndef CROSS_COMPILE
		$(error "CROSS_COMPILE is not set. Please set CROSS_COMPILE to your cross-compiler prefix.")
	endif

	# Set current directory
	PWD := $(shell pwd)

	# Automatically detect the number of CPU cores if CORES is not set
	ifndef CORES
		CORES := $(shell nproc)
	endif

	# Default BUILD_ID if not set
	ifndef BUILD_ID
		BUILD_ID := default
	endif

	# Default PUBLIC_DRIVER_DIR if not set
	ifndef PUBLIC_DRIVER_DIR
		PUBLIC_DRIVER_DIR := $(PWD)
	endif

	# Default address and image variables (remove or set as needed)
	# Example: address and image might be used for additional module parameters
	address :=
	image :=

# =============================================================================
# Build Targets Section
# =============================================================================

# Default target to build the kernel module and copy it to the public directory
all: build_module copy_module

# Build the kernel module
build_module:
	$(MAKE) -C $(KERNEL_SRC) \
		M=$(PWD) \
		ARCH=arm \
		CROSS_COMPILE=$(CROSS_COMPILE) \
		LOCALVERSION=-$(BUILD_ID) \
		-j$(CORES) modules

# Copy the compiled .ko file to the public directory
copy_module: build_module
	@echo "Copying .ko files to $(PUBLIC_DRIVER_DIR)..."
	cp -v *.ko $(PUBLIC_DRIVER_DIR)

# Clean build artifacts
clean:
	$(MAKE) -C $(KERNEL_SRC) \
		M=$(PWD) \
		ARCH=arm \
		CROSS_COMPILE=$(CROSS_COMPILE) \
		LOCALVERSION=-$(BUILD_ID) \
		clean

# =============================================================================
# End of Makefile
# =============================================================================

endif
default:
	# Trigger kernel build for this module
	${MAKE} -C ${KERNEL_SOURCE} M=${PWD} -j${CORES} ARCH=arm LOCALVERSION=-${BUILD} CROSS_COMPILE=${CC} ${address} ${image} modules
	# copy result to public folder
	cp *.ko ${PUBLIC_DRIVER_PWD}

clean:
	${MAKE} -C ${KERNEL_SOURCE} M=${PWD} clean


endif
