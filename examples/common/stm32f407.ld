/*
 * STM32F407VG linker script.
 *
 * FLASH holds everything that must survive power-off: the vector table, code,
 * constants and the initial values of .data. RAM holds the live copy of .data,
 * the zero-filled .bss, the heap and the stack.
 */

ENTRY(Reset_Handler)

MEMORY
{
    FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 1024K
    RAM   (rwx) : ORIGIN = 0x20000000, LENGTH = 128K
}

_estack = ORIGIN(RAM) + LENGTH(RAM);

INCLUDE stm32f407-sections.ld
