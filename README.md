# Morse Code LED Driver
## Overview

This Linux miscellaneous driver converts alphabetic text into Morse code signals via an LED. It uses a kernel FIFO (kfifo) to buffer Morse code symbols (dots, dashes, and spaces) for user-space access. The driver operates as a misc device, accessible at /dev/morse-code.
## Features

    Alphabetic Conversion: Converts A-Z letters (case-insensitive) to Morse code.
    LED Signaling: Flashes an LED according to Morse code timing rules.
    Word Separation: Supports word separation via spaces.
    Symbol Buffering: Buffers Morse symbols in a FIFO for read operations.
    Character Ignoring: Ignores non-alphabetic characters (except spaces).

## Prerequisites

    Linux Kernel: Must have misc device and LED trigger support.
    Build Tools: GCC, Make, and kernel headers.
    Permissions: Root/sudo access to load kernel modules.
    LED Configuration: An LED configured with the morse-code trigger (see LED Configuration).

## Installation
1. Please see the makefile's content and modify the following environment variables to fit your system:
    - KERNEL_SRC:
        - The path to your Linux kernel source. This directory should contain the kernel's Makefile and other necessary build files. It's crucial for compiling the module against the correct kernel version.

    - CROSS_COMPILE:
        - The prefix for your cross-compiler tools. For ARM architectures, this might be something like arm-linux-gnueabihf-. Ensure that the cross-compiler is installed and accessible in your system's PATH.

    - CORES:
        - The number of parallel jobs to run during compilation. Setting this to the number of available CPU cores can speed up the build process.

    - BUILD_ID:

        - An identifier for the build. This can help differentiate between different builds, especially when deploying to multiple environments.

    - PUBLIC_DRIVER_DIR:
        - The directory where the compiled .ko (kernel object) files will be copied after a successful build. This is useful for deploying the module to other systems or directories.

2. Open a terminal in the project directory and run:
    ```bash 
    make
    ```
3. Load the Module
    ```bash 
    sudo insmod morse-code.ko
    ```

1 Verify the Device File:

- Ensure that the device file is created at /dev/morse-code
    ```bash
    ls /dev/morse-code
    ```
    
    You should see /dev/morse-code listed.

## Usage
## Writing Text to Flash Morse Code

To convert text to Morse code and flash the LED:
```bash
echo "HELLO WORLD" > /dev/morse-code
```

    The LED will flash according to Morse code for the input text.
    Spaces between words will trigger a longer pause.

## Reading Morse Symbols from the FIFO

To read the sequence of Morse symbols from the FIFO:
```bash
cat /dev/morse-code
```

    Outputs the sequence of dots (.), dashes (-), letter spaces (single space), and word spaces (three spaces).

## Behavior Details
## Timing

    Dot: 200 ms LED on.
    Dash: 600 ms LED on.
    Intra-character Gap: 200 ms LED off (between dots/dashes).
    Inter-character Gap: 600 ms LED off (between letters).
    Word Gap: 1400 ms LED off (between words).

## Character Conversion

    Case-Insensitive: Letters are converted to uppercase.
    Ignored Characters: Non-alphabetic characters (except spaces) are ignored.
    Example: A → .-, B → -...

##  FIFO Behavior

    Capacity: Stores up to 1024 symbols (dots, dashes, spaces).
    Drain: Read operations drain the FIFO.
    Overflow: Overflow causes data loss and logs an error in the kernel log.

## LED Configuration

    Ensure an LED is Available:

    For example, led0.

Set the LED Trigger to Morse Code:
```bash
echo morse-code | sudo tee /sys/class/leds/led0/trigger
```
## Verify the LED Trigger:
```bash
cat /sys/class/leds/led0/trigger

You should see morse-code listed among the available triggers.
```
## Limitations

    Character Support: Supports only A-Z letters and spaces.
    FIFO Size: Fixed at 1024 bytes.
    Manual Setup: Requires manual LED trigger configuration.


