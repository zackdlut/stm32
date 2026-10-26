/*
 * STM32F103C8 linker script: 64 KB FLASH, 20 KB RAM.
 *
 * Only the memory sizes differ from ../stm32f407.ld; the section layout is
 * shared (found through -L../common).
 */

ENTRY(Reset_Handler)

MEMORY
{
    FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 64K
    RAM   (rwx) : ORIGIN = 0x20000000, LENGTH = 20K
}

_estack = ORIGIN(RAM) + LENGTH(RAM);

INCLUDE stm32f407-sections.ld
