RTOS := ../third_party/freertos-kernel

SRCS := main.c \
        $(RTOS)/tasks.c \
        $(RTOS)/queue.c \
        $(RTOS)/list.c \
        $(RTOS)/timers.c \
        $(RTOS)/portable/GCC/ARM_CM4F/port.c \
        $(RTOS)/portable/MemMang/heap_4.c

CPPFLAGS += -I$(RTOS)/include -I$(RTOS)/portable/GCC/ARM_CM4F
RUN_TIME := 1.3

include ../common/cmsis.mk
