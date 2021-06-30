This project is a demonstration of how to use the dsPIC33f and Explorer 16 board.
It enables primitive usage of the LCD, LED, UART, and push-button peripherals.
Interrupts enqueue events to be processed in the main event loop. No dynamic
allocation is performed.

## Requirements

1. MPLAB X IDE from Microchip, based on NetBeans
2. A dsPIC33FJ256GP710A microprocessor.
3. An Explorer 16 development board with LED, LCD, UART, and push-button
   peripherals.

## Features

*   *Separation of semantics from hardware*: Peripheral drivers don't interact
    directly with the hardware. Instead, the low-level routines they require
    are implemented by client code, making it easy to switch between different
    hardware interfaces without altering the driver software.
*   *Aggressively single-threaded*: The application is powered by a central
    event loop. Interrupt routines do the minimal amount of work necessary,
    communicating with the main thread through concurrency-safe data structures
    and the application event bus.

## Known issues

*   The FAT16 implementation has some unsolved bugs around reading and writing
    files.
