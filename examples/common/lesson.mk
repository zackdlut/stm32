# Shared build rules. A lesson Makefile sets SRCS (and optionally RUN_TIME)
# and then includes this file:
#
#     SRCS := main.c
#     include ../common/lesson.mk
#
# BOARD selects the target board:
#   disco  STM32F4 Discovery, STM32F407VG (default, the whole tutorial)
#   dkx    LCSC "Dikuoxing" STM32F103C8T6 (appendix C). Lessons that support
#          it keep their F103 sources in dkx/ and switch SRCS on $(BOARD).

COMMON_DIR := $(dir $(lastword $(MAKEFILE_LIST)))
COMMON_DIR := $(COMMON_DIR:/=)

BOARD ?= disco

PREFIX  ?= arm-none-eabi-
CC      := $(PREFIX)gcc
CXX     := $(PREFIX)g++
OBJCOPY := $(PREFIX)objcopy
OBJDUMP := $(PREFIX)objdump
SIZE    := $(PREFIX)size

TARGET   ?= firmware
RUN_TIME ?= 1

ifeq ($(BOARD),disco)
BUILD    ?= build
STARTUP  ?= $(COMMON_DIR)/startup.c
LDSCRIPT ?= $(COMMON_DIR)/stm32f407.ld
ARCH     := -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard
CPPFLAGS += -I$(COMMON_DIR) -I. -DSTM32F407xx
SIM_RESC := $(COMMON_DIR)/board.resc
SIM_UART := usart2
STIMULUS ?= stimulus.resc
else ifeq ($(BOARD),dkx)
BOARD_DIR := $(COMMON_DIR)/dkx
BUILD    ?= build-dkx
STARTUP  ?= $(BOARD_DIR)/startup.c
LDSCRIPT ?= $(BOARD_DIR)/stm32f103c8.ld
ARCH     := -mcpu=cortex-m3 -mthumb
CPPFLAGS += -I$(BOARD_DIR) -I. -DSTM32F103xB
SIM_RESC := $(BOARD_DIR)/board.resc
SIM_UART := usart1
# A dkx/stimulus.resc wins; button-only scripts are shared with the F407 version.
STIMULUS ?= $(firstword $(wildcard dkx/stimulus.resc) stimulus.resc)
else
$(error Unknown BOARD '$(BOARD)': use disco or dkx)
endif

SRCS += $(STARTUP) $(COMMON_DIR)/syscalls.c

OPT ?= -O2

CPPFLAGS += -MMD -MP
COMMON_FLAGS := $(ARCH) $(OPT) -g3 -Wall -Wextra -ffunction-sections -fdata-sections
CFLAGS   += $(COMMON_FLAGS) -std=c11
CXXFLAGS += $(COMMON_FLAGS) -std=c++17 -fno-exceptions -fno-rtti -fno-threadsafe-statics
LDFLAGS  += $(ARCH) -L$(COMMON_DIR) -T$(LDSCRIPT) --specs=nano.specs --specs=nosys.specs \
            -Wl,--gc-sections -Wl,-Map=$(BUILD)/$(TARGET).map -Wl,--print-memory-usage

# Link with gcc even for C++: g++ would add -lstdc++, and the bare-metal C++
# in lesson 14 needs nothing from that library.
LINKER := $(CC)

# One explicit rule per source file. A pattern rule would look for main.c in
# the current directory first and build the F407 file for BOARD=dkx.
obj_of = $(BUILD)/$(notdir $(basename $(1))).o
OBJS := $(foreach src,$(SRCS),$(call obj_of,$(src)))

define c_rule
$(call obj_of,$(1)): $(1) | $(BUILD)
	$$(CC) $$(CPPFLAGS) $$(CFLAGS) -c $$< -o $$@
endef

define cxx_rule
$(call obj_of,$(1)): $(1) | $(BUILD)
	$$(CXX) $$(CPPFLAGS) $$(CXXFLAGS) -c $$< -o $$@
endef

define asm_rule
$(call obj_of,$(1)): $(1) | $(BUILD)
	$$(CC) $$(ARCH) -c $$< -o $$@
endef

ELF := $(BUILD)/$(TARGET).elf

.PHONY: all clean run sim gdb disasm size

all: $(ELF) $(BUILD)/$(TARGET).bin

$(foreach src,$(filter %.c,$(SRCS)),$(eval $(call c_rule,$(src))))
$(foreach src,$(filter %.cpp,$(SRCS)),$(eval $(call cxx_rule,$(src))))
$(foreach src,$(filter %.s,$(SRCS)),$(eval $(call asm_rule,$(src))))

$(BUILD):
	@mkdir -p $@

$(ELF): $(OBJS) $(LDSCRIPT)
	$(LINKER) $(OBJS) $(LDFLAGS) -o $@
	$(SIZE) $@

$(BUILD)/$(TARGET).bin: $(ELF)
	$(OBJCOPY) -O binary $< $@

# Header dependencies written by -MMD: editing a .h rebuilds the objects using it.
-include $(OBJS:.o=.d)

SIM_ENV = RESC='$(SIM_RESC)' UART='$(SIM_UART)'

# Run headless in Renode for RUN_TIME seconds of virtual time and print the
# debug UART (USART2 on disco, USART1 on dkx).
run: all
	@$(SIM_ENV) LOG_FILTER='$(LOG_FILTER)' STIMULUS='$(STIMULUS)' $(COMMON_DIR)/run.sh $(ELF) $(RUN_TIME)

# Interactive Renode: debug UART on TCP port 3456, GDB server on port 3333.
# GUI=1 opens the monitor and a UART terminal as windows instead.
sim: all
	@$(SIM_ENV) GUI='$(GUI)' $(COMMON_DIR)/sim.sh $(ELF)

gdb: all
	gdb-multiarch -q $(ELF) -ex "target remote :3333"

disasm: all
	$(OBJDUMP) -d -S $(ELF) > $(BUILD)/$(TARGET).lst
	@echo "Disassembly written to $(BUILD)/$(TARGET).lst"

size: all
	$(SIZE) -A $(ELF)

clean:
	rm -rf $(BUILD)
