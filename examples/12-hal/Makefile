TP := ../third_party
HAL := $(TP)/stm32f4xx-hal-driver

ifeq ($(wildcard $(HAL)/Src/stm32f4xx_hal.c),)
$(error Vendor code missing: run ../third_party/fetch.sh first)
endif

STARTUP := $(TP)/cmsis-device-f4/Source/Templates/gcc/startup_stm32f407xx.s

SRCS := main.c stm32f4xx_hal_msp.c stm32f4xx_it.c \
        $(TP)/cmsis-device-f4/Source/Templates/system_stm32f4xx.c \
        $(HAL)/Src/stm32f4xx_hal.c \
        $(HAL)/Src/stm32f4xx_hal_cortex.c \
        $(HAL)/Src/stm32f4xx_hal_rcc.c \
        $(HAL)/Src/stm32f4xx_hal_rcc_ex.c \
        $(HAL)/Src/stm32f4xx_hal_gpio.c \
        $(HAL)/Src/stm32f4xx_hal_dma.c \
        $(HAL)/Src/stm32f4xx_hal_pwr.c \
        $(HAL)/Src/stm32f4xx_hal_flash.c \
        $(HAL)/Src/stm32f4xx_hal_uart.c

CPPFLAGS += -DUSE_HAL_DRIVER -DHSE_VALUE=8000000u \
            -I$(TP)/cmsis-core/Include \
            -I$(TP)/cmsis-device-f4/Include \
            -I$(HAL)/Inc

# ST's code trips a few of our extra warnings; they are not ours to fix.
CFLAGS += -Wno-unused-parameter

include ../common/lesson.mk
