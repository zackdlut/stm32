/* Internal flash erase/program through the FLASH interface registers. */
#ifndef FLASH_H
#define FLASH_H

#include <stddef.h>
#include <stdint.h>

void flash_unlock(void);
void flash_lock(void);

/* Sector number (0..11) containing addr, or -1. */
int flash_sector_of(uint32_t addr);

/* Both return 0 on success, or the FLASH_SR error bits. */
uint32_t flash_erase_sector(unsigned sector);
uint32_t flash_program(uint32_t addr, const uint32_t *src, size_t words);

#endif
