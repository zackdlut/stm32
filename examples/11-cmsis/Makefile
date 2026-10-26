TP := ../third_party

ifeq ($(BOARD),dkx)
CORE_H   := core_cm3.h
DEVICE   := $(TP)/cmsis-device-f1
STARTUP  := $(DEVICE)/Source/Templates/gcc/startup_stm32f103xb.s
SRCS     := dkx/main.c $(DEVICE)/Source/Templates/system_stm32f1xx.c
else
CORE_H   := core_cm4.h
DEVICE   := $(TP)/cmsis-device-f4
STARTUP  := $(DEVICE)/Source/Templates/gcc/startup_stm32f407xx.s
SRCS     := main.c $(DEVICE)/Source/Templates/system_stm32f4xx.c
endif

ifneq ($(words $(wildcard $(TP)/cmsis-core/Include/$(CORE_H) $(DEVICE)/Include)),2)
$(error Vendor code missing: run ../third_party/fetch.sh first)
endif

# Both boards have an 8 MHz crystal; ST's F4 default assumption is 25 MHz.
CPPFLAGS += -DHSE_VALUE=8000000u \
            -I$(TP)/cmsis-core/Include \
            -I$(DEVICE)/Include

include ../common/lesson.mk
