/*
 * Lesson 23: a bootloader.
 *
 * 1. If the staging slot holds a valid image newer than the installed one,
 *    copy it into the app slot and erase the staging slot.
 * 2. If the app slot holds a valid image, jump to it.
 * 3. Otherwise stay here with the red LED on.
 */
#include <stdbool.h>
#include <stdio.h>

#include "board.h"
#include "flash.h"
#include "image.h"
#include "uart.h"

static uint32_t crc32_hw(const uint32_t *p, uint32_t words)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_CRCEN;
    CRC->CR = CRC_CR_RESET;                 /* back to 0xFFFFFFFF */
    while (words--) {
        CRC->DR = *p++;
    }
    return CRC->DR;
}

static const image_header_t *header_at(uint32_t slot)
{
    return (const image_header_t *)slot;
}

static bool image_valid(const char *name, uint32_t slot, uint32_t slot_size)
{
    const image_header_t *h = header_at(slot);
    printf("  %s slot 0x%08lx: ", name, (unsigned long)slot);

    if (h->magic != IMAGE_MAGIC) {
        printf("no image\n");
        return false;
    }
    if (h->size == 0 || h->size % 4u || h->size > slot_size - IMAGE_HEADER_SIZE) {
        printf("bad size %lu\n", (unsigned long)h->size);
        return false;
    }
    const uint32_t *body = (const uint32_t *)(slot + IMAGE_HEADER_SIZE);
    uint32_t crc = crc32_hw(body, h->size / 4u);
    printf("version %lu, %lu bytes, crc 0x%08lx ", (unsigned long)h->version,
           (unsigned long)h->size, (unsigned long)h->crc32);
    if (crc != h->crc32) {
        printf("but computed 0x%08lx: CORRUPT\n", (unsigned long)crc);
        return false;
    }

    /* The first two vector table entries must make sense before we trust them.
     * Every image is linked to run from the app slot, wherever it is stored. */
    uint32_t sp = body[0], pc = body[1];
    if (sp < SRAM1_BASE || sp > SRAM1_BASE + 128u * 1024u || (sp & 3u) || !(pc & 1u) ||
        pc < APP_SLOT_ADDR + IMAGE_HEADER_SIZE || pc >= APP_SLOT_ADDR + APP_SLOT_SIZE) {
        printf("ok, but vectors SP=0x%08lx PC=0x%08lx are wrong\n", (unsigned long)sp,
               (unsigned long)pc);
        return false;
    }
    printf("ok\n");
    return true;
}

static bool install_update(void)
{
    const image_header_t *staged = header_at(STAGING_SLOT_ADDR);
    const image_header_t *installed = header_at(APP_SLOT_ADDR);
    uint32_t total = IMAGE_HEADER_SIZE + staged->size;

    if (installed->magic == IMAGE_MAGIC && staged->version <= installed->version) {
        printf("  staged version %lu is not newer than installed %lu, ignoring\n",
               (unsigned long)staged->version, (unsigned long)installed->version);
        return false;
    }
    printf("bootloader: installing version %lu\n", (unsigned long)staged->version);

    flash_unlock();
    int first = flash_sector_of(APP_SLOT_ADDR);
    int last = flash_sector_of(APP_SLOT_ADDR + APP_SLOT_SIZE - 1u);
    for (int s = first; s <= last; s++) {
        printf("  erase sector %d\n", s);
        if (flash_erase_sector((unsigned)s)) {
            printf("  erase failed\n");
            flash_lock();
            return false;
        }
    }
    printf("  program %lu bytes\n", (unsigned long)total);
    uint32_t err = flash_program(APP_SLOT_ADDR, (const uint32_t *)STAGING_SLOT_ADDR, total / 4u);
    if (!err) {
        /* Only drop the staged copy once the new app is known good, so a power
         * cut at any point leaves at least one complete image behind. */
        if (image_valid("new app", APP_SLOT_ADDR, APP_SLOT_SIZE)) {
            printf("  erase staging sector %d\n", flash_sector_of(STAGING_SLOT_ADDR));
            flash_erase_sector((unsigned)flash_sector_of(STAGING_SLOT_ADDR));
        } else {
            err = 1;
        }
    }
    flash_lock();
    return err == 0;
}

static void __attribute__((noreturn)) jump_to_app(uint32_t vector_table)
{
    const uint32_t *vt = (const uint32_t *)vector_table;
    printf("bootloader: jump to app, VTOR=0x%08lx SP=0x%08lx PC=0x%08lx\n\n",
           (unsigned long)vector_table, (unsigned long)vt[0], (unsigned long)vt[1]);
    uart_flush();

    /* Hand over a chip that looks as close to reset as possible. */
    __disable_irq();
    SysTick->CTRL = 0;
    for (unsigned i = 0; i < 8; i++) {
        NVIC->ICER[i] = 0xFFFFFFFFu;
        NVIC->ICPR[i] = 0xFFFFFFFFu;
    }
    RCC->APB1RSTR |= RCC_APB1RSTR_USART2RST;
    RCC->APB1RSTR &= ~RCC_APB1RSTR_USART2RST;

    SCB->VTOR = vector_table;               /* the app's interrupts now use its own table */
    __DSB();
    __ISB();
    __enable_irq();                         /* ST's startup code never re-enables them */

    __asm volatile("msr msp, %0\n\t"
                   "bx  %1"
                   :
                   : "r"(vt[0]), "r"(vt[1])
                   : "memory");
    __builtin_unreachable();
}

int main(void)
{
    board_init_leds();
    uart_init(HSI_HZ, 115200);
    printf("\nbootloader: started\n");

    if (image_valid("staging", STAGING_SLOT_ADDR, STAGING_SLOT_SIZE)) {
        install_update();
    }
    if (image_valid("app", APP_SLOT_ADDR, APP_SLOT_SIZE)) {
        jump_to_app(APP_SLOT_ADDR + IMAGE_HEADER_SIZE);
    }

    printf("bootloader: no valid app, waiting for a new image\n");
    led_on(LED_RED);
    for (;;) {
        __WFI();
    }
}
