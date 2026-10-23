# Shared build rules. A lesson Makefile sets SRCS (and optionally RUN_TIME)
# and then includes this file:
#
#     SRCS := main.c
#     include ../common/lesson.mk

COMMON_DIR := $(dir $(lastword $(MAKEFILE_LIST)))
COMMON_DIR := $(COMMON_DIR:/=)

PREFIX  ?= arm-none-eabi-
CC      := $(PREFIX)gcc
CXX     := $(PREFIX)g++
OBJCOPY := $(PREFIX)objcopy
OBJDUMP := $(PREFIX)objdump
SIZE    := $(PREFIX)size

TARGET   ?= firmware
BUILD    ?= build
RUN_TIME ?= 1

STARTUP ?= $(COMMON_DIR)/startup.c
SRCS += $(STARTUP) $(COMMON_DIR)/syscalls.c
LDSCRIPT ?= $(COMMON_DIR)/stm32f407.ld

ARCH := -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard

OPT ?= -O2

CPPFLAGS += -I$(COMMON_DIR) -I. -DSTM32F407xx
COMMON_FLAGS := $(ARCH) $(OPT) -g3 -Wall -Wextra -ffunction-sections -fdata-sections
CFLAGS   += $(COMMON_FLAGS) -std=c11
CXXFLAGS += $(COMMON_FLAGS) -std=c++17 -fno-exceptions -fno-rtti -fno-threadsafe-statics
LDFLAGS  += $(ARCH) -T$(LDSCRIPT) --specs=nano.specs --specs=nosys.specs \
            -Wl,--gc-sections -Wl,-Map=$(BUILD)/$(TARGET).map -Wl,--print-memory-usage

# Link with gcc even for C++: g++ would add -lstdc++, and the bare-metal C++
# in lesson 14 needs nothing from that library.
LINKER := $(CC)

OBJS := $(addprefix $(BUILD)/,$(notdir $(addsuffix .o,$(basename $(SRCS)))))
vpath %.c   $(sort $(dir $(SRCS)))
vpath %.cpp $(sort $(dir $(SRCS)))
vpath %.s   $(sort $(dir $(SRCS)))

ELF := $(BUILD)/$(TARGET).elf

.PHONY: all clean run sim gdb disasm size

all: $(ELF) $(BUILD)/$(TARGET).bin

$(BUILD):
	@mkdir -p $@

$(BUILD)/%.o: %.c | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/%.o: %.cpp | $(BUILD)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

$(BUILD)/%.o: %.s | $(BUILD)
	$(CC) $(ARCH) -c $< -o $@

$(ELF): $(OBJS) $(LDSCRIPT)
	$(LINKER) $(OBJS) $(LDFLAGS) -o $@
	$(SIZE) $@

$(BUILD)/$(TARGET).bin: $(ELF)
	$(OBJCOPY) -O binary $< $@

# Run headless in Renode for RUN_TIME seconds of virtual time and print USART2.
run: all
	@$(COMMON_DIR)/run.sh $(ELF) $(RUN_TIME)

# Interactive Renode: USART2 on TCP port 3456, GDB server on port 3333.
sim: all
	@$(COMMON_DIR)/sim.sh $(ELF)

gdb: all
	gdb-multiarch -q $(ELF) -ex "target remote :3333"

disasm: all
	$(OBJDUMP) -d -S $(ELF) > $(BUILD)/$(TARGET).lst
	@echo "Disassembly written to $(BUILD)/$(TARGET).lst"

size: all
	$(SIZE) -A $(ELF)

clean:
	rm -rf $(BUILD)
