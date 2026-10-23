TP := ../third_party

ifeq ($(wildcard $(TP)/cmsis-core/Include/core_cm4.h),)
$(error Vendor code missing: run ../third_party/fetch.sh first)
endif

STARTUP := $(TP)/cmsis-device-f4/Source/Templates/gcc/startup_stm32f407xx.s
SRCS := main.c $(TP)/cmsis-device-f4/Source/Templates/system_stm32f4xx.c

# The Discovery board has an 8 MHz crystal; ST's default assumption is 25 MHz.
CPPFLAGS += -DHSE_VALUE=8000000u \
            -I$(TP)/cmsis-core/Include \
            -I$(TP)/cmsis-device-f4/Include

include ../common/lesson.mk
