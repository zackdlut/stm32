TOOLCHAIN ?= $(HOME)/.local/arm-none-eabi/usr/bin/arm-none-eabi-
CC := $(TOOLCHAIN)gcc
SIZE := $(TOOLCHAIN)size

CFLAGS := -mcpu=cortex-m4 -mthumb -mfloat-abi=soft -ffreestanding -fno-builtin -Os -Wall -Wextra -g
LDFLAGS := -nostdlib -T linker.ld -Wl,-Map=firmware.map

.PHONY: all run clean

all: firmware.elf

firmware.elf: startup.s main.c linker.ld
	$(CC) $(CFLAGS) startup.s main.c $(LDFLAGS) -o $@
	$(SIZE) $@

# Without this rule, make's built-in `%: %.sh` would copy run.sh to ./run.
run: firmware.elf
	./run.sh

clean:
	rm -f firmware.elf firmware.map uart.log renode.log
