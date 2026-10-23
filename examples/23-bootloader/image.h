/*
 * Flash layout and image header shared by the bootloader, the app and
 * tools/sign.py (which writes the header after the app is linked).
 *
 *   0x08000000  sectors 0-1   32 KB  bootloader
 *   0x08008000  sectors 2-4   96 KB  app slot:     header | vector table | code
 *   0x08020000  sector  5    128 KB  staging slot: a new image waiting to be installed
 */
#ifndef IMAGE_H
#define IMAGE_H

#include <stdint.h>

#define APP_SLOT_ADDR      0x08008000u
#define APP_SLOT_SIZE      (96u * 1024u)
#define STAGING_SLOT_ADDR  0x08020000u
#define STAGING_SLOT_SIZE  (128u * 1024u)

/* The vector table must be aligned to its size rounded up to a power of two:
 * 16 + 82 entries = 392 bytes -> 512. The header fills the gap before it. */
#define IMAGE_HEADER_SIZE  0x200u
#define IMAGE_MAGIC        0x31474D49u          /* "IMG1" in memory */

typedef struct {
    uint32_t magic;
    uint32_t version;
    uint32_t size;      /* bytes after the header, a multiple of 4 */
    uint32_t crc32;     /* STM32 hardware CRC (CRC-32/MPEG-2) of those bytes */
} image_header_t;

#endif
