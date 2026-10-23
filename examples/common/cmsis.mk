# Build rules for lessons that use ST's CMSIS device headers (lesson 11 on).
# A lesson Makefile sets SRCS and includes this file instead of lesson.mk:
#
#     SRCS := main.c
#     include ../common/cmsis.mk

CMSIS_MK_DIR := $(dir $(lastword $(MAKEFILE_LIST)))
CMSIS_MK_DIR := $(CMSIS_MK_DIR:/=)
TP := $(CMSIS_MK_DIR)/../third_party

ifeq ($(wildcard $(TP)/cmsis-core/Include/core_cm4.h),)
$(error Vendor code missing: run examples/third_party/fetch.sh first)
endif

STARTUP ?= $(TP)/cmsis-device-f4/Source/Templates/gcc/startup_stm32f407xx.s
SRCS += $(TP)/cmsis-device-f4/Source/Templates/system_stm32f4xx.c \
        $(CMSIS_MK_DIR)/cmsis-drivers/uart.c \
        $(CMSIS_MK_DIR)/cmsis-drivers/tick.c

CPPFLAGS += -DHSE_VALUE=8000000u \
            -I$(CMSIS_MK_DIR)/cmsis-drivers \
            -I$(TP)/cmsis-core/Include \
            -I$(TP)/cmsis-device-f4/Include

include $(CMSIS_MK_DIR)/lesson.mk
