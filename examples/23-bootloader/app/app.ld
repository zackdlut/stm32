/*
 * App: linked to run from the app slot, just after the 512-byte header that
 * tools/sign.py prepends. Nothing else differs from a stand-alone program.
 */
ENTRY(Reset_Handler)

MEMORY
{
    FLASH (rx)  : ORIGIN = 0x08008200, LENGTH = 96K - 0x200
    RAM   (rwx) : ORIGIN = 0x20000000, LENGTH = 128K
}

_estack = ORIGIN(RAM) + LENGTH(RAM);

INCLUDE stm32f407-sections.ld
