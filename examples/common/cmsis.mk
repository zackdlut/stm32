# Build rules for lessons that use ST's CMSIS device headers (lesson 11 on).
# A lesson Makefile sets SRCS and includes this file instead of lesson.mk:
#
#     SRCS := main.c
#     include ../common/cmsis.mk
#
# BOARD=dkx switches to the STM32F103 headers and common/dkx/cmsis-drivers.

CMSIS_MK_DIR := $(dir $(lastword $(MAKEFILE_LIST)))
CMSIS_MK_DIR := $(CMSIS_MK_DIR:/=)
TP := $(CMSIS_MK_DIR)/../third_party

BOARD ?= disco

ifeq ($(BOARD),dkx)
CMSIS_CORE_H   := core_cm3.h
CMSIS_DEVICE   := $(TP)/cmsis-device-f1
CMSIS_DRIVERS  := $(CMSIS_MK_DIR)/dkx/cmsis-drivers
STARTUP        ?= $(CMSIS_DEVICE)/Source/Templates/gcc/startup_stm32f103xb.s
CMSIS_SYSTEM_C := $(CMSIS_DEVICE)/Source/Templates/system_stm32f1xx.c
else
CMSIS_CORE_H   := core_cm4.h
CMSIS_DEVICE   := $(TP)/cmsis-device-f4
CMSIS_DRIVERS  := $(CMSIS_MK_DIR)/cmsis-drivers
STARTUP        ?= $(CMSIS_DEVICE)/Source/Templates/gcc/startup_stm32f407xx.s
CMSIS_SYSTEM_C := $(CMSIS_DEVICE)/Source/Templates/system_stm32f4xx.c
endif

ifneq ($(words $(wildcard $(TP)/cmsis-core/Include/$(CMSIS_CORE_H) $(CMSIS_DEVICE)/Include)),2)
$(error Vendor code missing: run examples/third_party/fetch.sh first)
endif

SRCS += $(CMSIS_SYSTEM_C) \
        $(CMSIS_DRIVERS)/uart.c \
        $(CMSIS_DRIVERS)/tick.c

CPPFLAGS += -DHSE_VALUE=8000000u \
            -I$(CMSIS_DRIVERS) \
            -I$(TP)/cmsis-core/Include \
            -I$(CMSIS_DEVICE)/Include

include $(CMSIS_MK_DIR)/lesson.mk
