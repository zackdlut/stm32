FAULT ?= 1

ifeq ($(BOARD),dkx)
SRCS := dkx/main.c ../common/dkx/drivers/uart.c
BUILD := build-dkx/fault$(FAULT)
GDB_SCRIPT := dkx/debug.gdb
else
SRCS := main.c ../common/drivers/uart.c
# One build directory per variant, so switching FAULT always rebuilds.
BUILD := build/fault$(FAULT)
GDB_SCRIPT := debug.gdb
endif
RUN_TIME := 0.1
# -Og keeps variables and call structure close to the source for debugging.
OPT := -Og

CPPFLAGS += -DFAULT_KIND=$(FAULT) -DELF_PATH='"$(BUILD)/firmware.elf"'

include ../common/lesson.mk

.PHONY: gdb-demo
gdb-demo: all
	@$(SIM_ENV) GDB_SCRIPT='$(GDB_SCRIPT)' ./gdb-demo.sh $(ELF)
