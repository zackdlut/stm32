/*
 * Lesson 3, Dikuoxing STM32F103C8T6 version: what happens before main().
 *
 * Same program as the F407 one; only the memory sizes differ (64 KB FLASH,
 * 20 KB RAM), so _estack is 0x20005000 here.
 */
#include <stdio.h>
#include <stdlib.h>

#include "drivers/uart.h"
#include "stm32f103.h"

int g_initialized = 0x12345678;            /* .data: value stored in FLASH, copied to RAM */
int g_zeroed;                              /* .bss:  zero-filled by Reset_Handler */
static int s_counter = 42;                 /* .data, file scope only */
const char g_message[] = "I live in FLASH"; /* .rodata: never copied */

static int s_constructor_ran;

/* Runs from __libc_init_array() before main(), like a C++ global constructor. */
__attribute__((constructor)) static void before_main(void)
{
    s_constructor_ran = 1;
}

extern uint32_t _sidata, _sdata, _edata, _sbss, _ebss, _estack;
extern const uint32_t g_vector_table[];
extern void Reset_Handler(void);

static inline uint32_t read_sp(void)
{
    uint32_t sp;
    __asm__ volatile("mov %0, sp" : "=r"(sp));
    return sp;
}

static void row(const char *name, const void *addr, const char *section)
{
    printf("  %-16s @ 0x%08lx  %s\n", name, (unsigned long)addr, section);
}

int main(void)
{
    int local = 7;
    uart_init(8000000u, 115200);

    printf("\n== Where variables live ==\n");
    row("g_vector_table", g_vector_table, ".isr_vector (FLASH)");
    row("main", (const void *)main, ".text       (FLASH)");
    row("g_message", g_message, ".rodata     (FLASH)");
    row("g_initialized", &g_initialized, ".data       (RAM)");
    row("s_counter", &s_counter, ".data       (RAM)");
    row("g_zeroed", &g_zeroed, ".bss        (RAM)");
    row("local", &local, "stack       (RAM)");

    printf("\n== Values set up by Reset_Handler ==\n");
    printf("  g_initialized = 0x%08x (copied from FLASH)\n", g_initialized);
    printf("  s_counter     = %d\n", s_counter);
    printf("  g_zeroed      = %d (cleared)\n", g_zeroed);
    printf("  constructor   = %s\n", s_constructor_ran ? "ran before main" : "did not run");

    printf("\n== Linker script symbols ==\n");
    printf("  _sidata = 0x%08lx  initial values of .data in FLASH\n", (unsigned long)&_sidata);
    printf("  _sdata  = 0x%08lx  _edata = 0x%08lx  (.data %lu bytes)\n",
           (unsigned long)&_sdata, (unsigned long)&_edata,
           (unsigned long)((uint8_t *)&_edata - (uint8_t *)&_sdata));
    printf("  _sbss   = 0x%08lx  _ebss  = 0x%08lx  (.bss %lu bytes)\n",
           (unsigned long)&_sbss, (unsigned long)&_ebss,
           (unsigned long)((uint8_t *)&_ebss - (uint8_t *)&_sbss));
    printf("  _estack = 0x%08lx  top of RAM\n", (unsigned long)&_estack);

    printf("\n== Vector table ==\n");
    printf("  word 0 (initial SP)    = 0x%08lx\n", (unsigned long)g_vector_table[0]);
    printf("  word 1 (Reset_Handler) = 0x%08lx (odd: Thumb bit)\n", (unsigned long)g_vector_table[1]);
    printf("  &Reset_Handler         = 0x%08lx\n", (unsigned long)Reset_Handler);

    printf("\n== Stack and heap ==\n");
    printf("  SP now      = 0x%08lx (%lu bytes used)\n", (unsigned long)read_sp(),
           (unsigned long)((uint32_t)&_estack - read_sp()));
    void *p = malloc(64);
    printf("  malloc(64)  = 0x%08lx (heap starts after .bss)\n", (unsigned long)p);
    free(p);

    for (;;) {
    }
}
