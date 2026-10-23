/*
 * Lesson 13: debugging with GDB and diagnosing faults.
 *
 * The program averages a few "sensor" readings. Build with FAULT=n to plant a
 * bug; the fault handler then prints the stacked registers and the fault
 * status bits, and tells you how to map the PC back to a source line.
 *
 *   make run FAULT=0   no bug
 *   make run FAULT=1   integer divide by zero          (UsageFault, DIVBYZERO)
 *   make run FAULT=2   call through a bad function ptr  (UsageFault, INVSTATE)
 *   make run FAULT=3   undefined instruction            (UsageFault, UNDEFINSTR)
 *   make run FAULT=4   same as 1, fault handlers off    (escalates to HardFault)
 */
#include <stdio.h>

#include "drivers/board.h"
#include "drivers/uart.h"

#ifndef FAULT_KIND
#define FAULT_KIND 1
#endif

#ifndef ELF_PATH
#define ELF_PATH "firmware.elf"
#endif

static int32_t g_samples[8] = {12, 15, 11, 14, 13, 16, 12, 15};
static volatile uint32_t g_valid_count = 8;

/* ---- Application code with planted bugs ---------------------------------- */
__attribute__((noinline)) static int32_t compute_average(const int32_t *samples, uint32_t count)
{
    int32_t sum = 0;
    for (uint32_t i = 0; i < count; i++) {
        sum += samples[i];
    }
    return sum / (int32_t)count;     /* count == 0 -> SDIV by zero */
}

typedef void (*callback_t)(int32_t);

static void report(int32_t value)
{
    printf("average = %ld\n", (long)value);
}

__attribute__((noinline)) static void process(void)
{
    callback_t cb = report;

#if FAULT_KIND == 1 || FAULT_KIND == 4
    g_valid_count = 0;               /* bug: "no valid samples" */
#elif FAULT_KIND == 2
    cb = (callback_t)((uintptr_t)report & ~1u);   /* bug: Thumb bit lost */
#elif FAULT_KIND == 3
    __asm__ volatile("udf #0");      /* bug: an undefined instruction */
#endif

    int32_t avg = compute_average(g_samples, g_valid_count);
    cb(avg);
}

/* ---- Fault reporting ----------------------------------------------------- */

/* What the core pushes on exception entry, lowest address first. */
typedef struct {
    uint32_t r0, r1, r2, r3, r12, lr, pc, xpsr;
} exception_frame_t;

static const char *const k_cfsr_bits[32] = {
    [0] = "IACCVIOL", [1] = "DACCVIOL", [3] = "MUNSTKERR", [4] = "MSTKERR", [5] = "MLSPERR", [7] = "MMARVALID",
    [8] = "IBUSERR", [9] = "PRECISERR", [10] = "IMPRECISERR", [11] = "UNSTKERR", [12] = "STKERR",
    [13] = "LSPERR", [15] = "BFARVALID",
    [16] = "UNDEFINSTR", [17] = "INVSTATE", [18] = "INVPC", [19] = "NOCP", [24] = "UNALIGNED", [25] = "DIVBYZERO",
};

static const char *exception_name(uint32_t n)
{
    switch (n) {
    case 3: return "HardFault";
    case 4: return "MemManage";
    case 5: return "BusFault";
    case 6: return "UsageFault";
    default: return "exception";
    }
}

void fault_report(const exception_frame_t *frame, uint32_t exc_return)
{
    uint32_t active = SCB->ICSR & 0x1FFu;
    uint32_t cfsr = SCB->CFSR;
    uint32_t hfsr = SCB->HFSR;

    led_on(LED_RED);
    printf("\n*** %s (exception %lu) ***\n", exception_name(active), (unsigned long)active);
    printf("  r0  = 0x%08lx  r1 = 0x%08lx  r2 = 0x%08lx  r3 = 0x%08lx\n",
           (unsigned long)frame->r0, (unsigned long)frame->r1, (unsigned long)frame->r2, (unsigned long)frame->r3);
    printf("  r12 = 0x%08lx  lr = 0x%08lx  pc = 0x%08lx  xpsr = 0x%08lx\n",
           (unsigned long)frame->r12, (unsigned long)frame->lr, (unsigned long)frame->pc, (unsigned long)frame->xpsr);
    printf("  EXC_RETURN = 0x%08lx (%s stack)\n", (unsigned long)exc_return, (exc_return & 4u) ? "process" : "main");

    printf("  CFSR = 0x%08lx:", (unsigned long)cfsr);
    for (int bit = 0; bit < 32; bit++) {
        if ((cfsr & (1u << bit)) && k_cfsr_bits[bit]) {
            printf(" %s", k_cfsr_bits[bit]);
        }
    }
    printf("\n  HFSR = 0x%08lx%s\n", (unsigned long)hfsr, (hfsr & (1u << 30)) ? ": FORCED (escalated)" : "");
    if (cfsr & (1u << 15)) {
        printf("  BFAR = 0x%08lx\n", (unsigned long)SCB->BFAR);
    }
    if (cfsr & (1u << 7)) {
        printf("  MMFAR = 0x%08lx\n", (unsigned long)SCB->MMFAR);
    }
    printf("  find the line: arm-none-eabi-addr2line -f -e %s 0x%08lx\n", ELF_PATH,
           (unsigned long)frame->pc);

    for (;;) {
    }
}

/*
 * The handler must not touch the stack before reading it, so it is naked.
 * Bit 2 of EXC_RETURN (in LR) tells which stack pointer held the frame.
 */
__attribute__((naked)) void HardFault_Handler(void)
{
    __asm__ volatile(
        "tst   lr, #4      \n"
        "ite   eq          \n"
        "mrseq r0, msp     \n"
        "mrsne r0, psp     \n"
        "mov   r1, lr      \n"
        "b     fault_report\n");
}

void UsageFault_Handler(void) __attribute__((naked, alias("HardFault_Handler")));
void BusFault_Handler(void) __attribute__((naked, alias("HardFault_Handler")));
void MemManage_Handler(void) __attribute__((naked, alias("HardFault_Handler")));

int main(void)
{
    board_init_leds();
    uart_init(HSI_HZ, 115200);

#if FAULT_KIND != 4
    /* Give each fault class its own handler instead of escalating to HardFault. */
    SCB->SHCSR |= SCB_SHCSR_USGFAULTENA | SCB_SHCSR_BUSFAULTENA | SCB_SHCSR_MEMFAULTENA;
#endif
    SCB->CCR |= SCB_CCR_DIV_0_TRP;   /* integer division by zero traps instead of returning 0 */

    printf("lesson 13, FAULT_KIND=%d\n", FAULT_KIND);
    process();
    printf("finished without a fault\n");

    for (;;) {
    }
}
