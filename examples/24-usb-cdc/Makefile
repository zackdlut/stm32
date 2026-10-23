# Builds for real hardware only (Renode has no OTG_FS model): make, then
# flash build/firmware.elf as in chapter 15.
TUSB := ../third_party/tinyusb/src

SRCS := main.c usb_descriptors.c \
        $(TUSB)/tusb.c \
        $(TUSB)/common/tusb_fifo.c \
        $(TUSB)/device/usbd.c \
        $(TUSB)/class/cdc/cdc_device.c \
        $(TUSB)/portable/synopsys/dwc2/dcd_dwc2.c \
        $(TUSB)/portable/synopsys/dwc2/dwc2_common.c

CPPFLAGS += -I$(TUSB)
RUN_TIME := 0.05

include ../common/cmsis.mk
