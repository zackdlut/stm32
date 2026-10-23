#!/usr/bin/env python3
"""Turn a linked app binary into an image the bootloader accepts.

    sign.py <app.bin> <version> <out.img>

out.img = 512-byte header (magic, version, size, crc32, then 0xFF padding)
          followed by app.bin padded with 0xFF to a multiple of 4 bytes.
The layout matches image_header_t in image.h.
"""
import struct
import sys

IMAGE_MAGIC = 0x31474D49
IMAGE_HEADER_SIZE = 0x200


def crc32_stm32(data: bytes) -> int:
    """CRC-32/MPEG-2 over little-endian 32-bit words, as the STM32 CRC unit computes it."""
    crc = 0xFFFFFFFF
    for (word,) in struct.iter_unpack("<I", data):
        crc ^= word
        for _ in range(32):
            crc = ((crc << 1) ^ 0x04C11DB7) if crc & 0x80000000 else (crc << 1)
            crc &= 0xFFFFFFFF
    return crc


def main() -> None:
    src, version, dst = sys.argv[1], int(sys.argv[2]), sys.argv[3]
    body = open(src, "rb").read()
    body += b"\xff" * (-len(body) % 4)
    crc = crc32_stm32(body)
    header = struct.pack("<4I", IMAGE_MAGIC, version, len(body), crc)
    header = header.ljust(IMAGE_HEADER_SIZE, b"\xff")
    with open(dst, "wb") as f:
        f.write(header + body)
    print(f"{dst}: version {version}, {len(body)} bytes, crc 0x{crc:08x}")


if __name__ == "__main__":
    main()
