FAULT ?= 1

SRCS := main.c ../common/drivers/uart.c
RUN_TIME := 0.1
# -Og keeps variables and call structure close to the source for debugging.
OPT := -Og

# One build directory per variant, so switching FAULT always rebuilds.
BUILD := build/fault$(FAULT)

CPPFLAGS += -DFAULT_KIND=$(FAULT) -DELF_PATH='"$(BUILD)/firmware.elf"'

include ../common/lesson.mk

.PHONY: gdb-demo
gdb-demo: all
	./gdb-demo.sh $(ELF)
