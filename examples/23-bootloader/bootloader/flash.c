#include "flash.h"

#include "stm32f4xx.h"

/* Bit 1 is OPERR in RM0090; ST's header calls it SOP. */
#define FLASH_ERRORS \
    (FLASH_SR_SOP | FLASH_SR_WRPERR | FLASH_SR_PGAERR | FLASH_SR_PGPERR | FLASH_SR_PGSERR)

void flash_unlock(void)
{
    if (FLASH->CR & FLASH_CR_LOCK) {
        FLASH->KEYR = 0x45670123u;          /* wrong order or value locks it until reset */
        FLASH->KEYR = 0xCDEF89ABu;
    }
}

void flash_lock(void)
{
    FLASH->CR |= FLASH_CR_LOCK;
}

static void wait_ready(void)
{
    while (FLASH->SR & FLASH_SR_BSY) {
    }
}

int flash_sector_of(uint32_t addr)
{
    uint32_t off = addr - FLASH_BASE;
    if (off < 64u * 1024u) {
        return (int)(off / (16u * 1024u));              /* 0-3: 16 KB */
    }
    if (off < 128u * 1024u) {
        return 4;                                       /* 4: 64 KB */
    }
    if (off < 1024u * 1024u) {
        return 4 + (int)(off / (128u * 1024u));         /* 5-11: 128 KB */
    }
    return -1;
}

uint32_t flash_erase_sector(unsigned sector)
{
    wait_ready();
    FLASH->SR = FLASH_ERRORS;                           /* rc_w1: write 1 to clear */
    /* PSIZE = x32: erase/program 32 bits at a time, valid for VDD 2.7-3.6 V. */
    FLASH->CR = FLASH_CR_PSIZE_1 | FLASH_CR_SER | (sector << FLASH_CR_SNB_Pos);
    FLASH->CR |= FLASH_CR_STRT;
    wait_ready();                                       /* 16 KB: ~0.5 s, 128 KB: ~1-2 s */
    FLASH->CR &= ~FLASH_CR_SER;
    return FLASH->SR & FLASH_ERRORS;
}

uint32_t flash_program(uint32_t addr, const uint32_t *src, size_t words)
{
    uint32_t err = 0;
    wait_ready();
    FLASH->SR = FLASH_ERRORS;
    FLASH->CR = FLASH_CR_PSIZE_1 | FLASH_CR_PG;
    for (size_t i = 0; i < words && !err; i++) {
        *(volatile uint32_t *)(addr + 4u * i) = src[i];   /* PG=1 turns this into a program */
        wait_ready();
        err = FLASH->SR & FLASH_ERRORS;
    }
    FLASH->CR &= ~FLASH_CR_PG;
    return err;
}
