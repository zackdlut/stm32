/*
 * USB descriptors: the data the host reads during enumeration to learn what
 * this device is. TinyUSB calls the tud_descriptor_*_cb() functions when the
 * host sends GET_DESCRIPTOR requests on endpoint 0.
 */
#include <string.h>

#include "tusb.h"

#define USB_VID 0xCAFE      /* TinyUSB's test VID: fine on your desk, not for a product */
#define USB_PID 0x4024

/* ---- device descriptor: who am I ----------------------------------------- */
static const tusb_desc_device_t desc_device = {
    .bLength = sizeof(tusb_desc_device_t),
    .bDescriptorType = TUSB_DESC_DEVICE,
    .bcdUSB = 0x0200,
    /* CDC uses two interfaces; an Interface Association Descriptor groups them,
     * which the spec requires to be announced with these three values. */
    .bDeviceClass = TUSB_CLASS_MISC,
    .bDeviceSubClass = MISC_SUBCLASS_COMMON,
    .bDeviceProtocol = MISC_PROTOCOL_IAD,
    .bMaxPacketSize0 = CFG_TUD_ENDPOINT0_SIZE,
    .idVendor = USB_VID,
    .idProduct = USB_PID,
    .bcdDevice = 0x0100,
    .iManufacturer = 1,
    .iProduct = 2,
    .iSerialNumber = 3,
    .bNumConfigurations = 1,
};

const uint8_t *tud_descriptor_device_cb(void)
{
    return (const uint8_t *)&desc_device;
}

/* ---- configuration descriptor: interfaces and endpoints ------------------ */
enum { ITF_NUM_CDC = 0, ITF_NUM_CDC_DATA, ITF_NUM_TOTAL };

#define EPNUM_CDC_NOTIF 0x81    /* interrupt IN: serial state notifications */
#define EPNUM_CDC_OUT   0x02    /* bulk OUT: host -> device data */
#define EPNUM_CDC_IN    0x82    /* bulk IN:  device -> host data */

#define CONFIG_TOTAL_LEN (TUD_CONFIG_DESC_LEN + TUD_CDC_DESC_LEN)

static const uint8_t desc_configuration[] = {
    /* config number, interface count, string index, total length, attributes, max power mA */
    TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, CONFIG_TOTAL_LEN, 0x00, 100),
    /* interface number, string index, notification EP and size, data EPs (out, in) and size */
    TUD_CDC_DESCRIPTOR(ITF_NUM_CDC, 4, EPNUM_CDC_NOTIF, 8, EPNUM_CDC_OUT, EPNUM_CDC_IN, 64),
};

const uint8_t *tud_descriptor_configuration_cb(uint8_t index)
{
    (void)index;
    return desc_configuration;
}

/* ---- string descriptors: UTF-16LE text shown by the host ----------------- */
static const char *const strings[] = {
    NULL,                       /* 0: language list, handled below */
    "STM32 tutorial",           /* 1: manufacturer */
    "Lesson 24 CDC echo",       /* 2: product */
    "0001",                     /* 3: serial number (a real product reads the chip UID) */
    "Lesson 24 serial port",    /* 4: CDC interface */
};

const uint16_t *tud_descriptor_string_cb(uint8_t index, uint16_t langid)
{
    static uint16_t buf[32 + 1];
    size_t n;
    (void)langid;

    if (index == 0) {
        buf[1] = 0x0409;        /* English (United States) */
        n = 1;
    } else {
        if (index >= sizeof strings / sizeof strings[0]) {
            return NULL;
        }
        n = strlen(strings[index]);
        if (n > 32) {
            n = 32;
        }
        for (size_t i = 0; i < n; i++) {
            buf[1 + i] = (uint8_t)strings[index][i];
        }
    }
    buf[0] = (uint16_t)((TUSB_DESC_STRING << 8) | (2 * n + 2));   /* length, type */
    return buf;
}
