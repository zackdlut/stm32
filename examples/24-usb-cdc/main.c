/*
 * Lesson 24: USB device, CDC-ACM class (a virtual serial port) with TinyUSB.
 *
 * Plug the board's micro-USB connector (CN5, OTG_FS) into a PC: it appears as
 * /dev/ttyACM0 on Linux or a COM port on Windows. Every line you type comes
 * back upper-cased. USART2 logs USB events.
 *
 * Needs real hardware: Renode has no model of the STM32 OTG_FS controller.
 */
#include <ctype.h>
#include <stdio.h>

#include "board.h"
#include "tick.h"
#include "tusb.h"
#include "uart.h"

#define SYSCLK_HZ 168000000u
#define PCLK1_HZ  (SYSCLK_HZ / 4u)

/*
 * HSE 8 MHz -> /M=8 -> 1 MHz -> xN=336 -> 336 MHz VCO
 *   /P=2 -> 168 MHz SYSCLK
 *   /Q=7 ->  48 MHz for USB (the USB core needs exactly 48 MHz)
 */
static void clock_init_168mhz(void)
{
    RCC->CR |= RCC_CR_HSEON;
    while (!(RCC->CR & RCC_CR_HSERDY)) {
    }
    RCC->APB1ENR |= RCC_APB1ENR_PWREN;
    PWR->CR |= PWR_CR_VOS;                              /* scale 1: required above 144 MHz */

    RCC->PLLCFGR = (8u << RCC_PLLCFGR_PLLM_Pos) | (336u << RCC_PLLCFGR_PLLN_Pos) |
                   (0u << RCC_PLLCFGR_PLLP_Pos) /* /2 */ | RCC_PLLCFGR_PLLSRC_HSE |
                   (7u << RCC_PLLCFGR_PLLQ_Pos);
    RCC->CR |= RCC_CR_PLLON;
    while (!(RCC->CR & RCC_CR_PLLRDY)) {
    }

    FLASH->ACR = FLASH_ACR_LATENCY_5WS | FLASH_ACR_PRFTEN | FLASH_ACR_ICEN | FLASH_ACR_DCEN;
    RCC->CFGR = RCC_CFGR_HPRE_DIV1 | RCC_CFGR_PPRE1_DIV4 | RCC_CFGR_PPRE2_DIV2 | RCC_CFGR_SW_PLL;
    while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL) {
    }
    SystemCoreClock = SYSCLK_HZ;
}

static void usb_pins_init(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    gpio_set_af(GPIOA, 11, 10);                         /* PA11 = DM, AF10 OTG_FS */
    gpio_set_af(GPIOA, 12, 10);                         /* PA12 = DP */
    GPIOA->OSPEEDR |= (3u << (11 * 2)) | (3u << (12 * 2));
    gpio_set_mode(GPIOA, 9, GPIO_MODE_INPUT);           /* PA9 = VBUS sense (5 V from host) */
    RCC->AHB2ENR |= RCC_AHB2ENR_OTGFSEN;
}

void OTG_FS_IRQHandler(void)
{
    tud_int_handler(0);                                 /* TinyUSB records events, tud_task() handles them */
}

/* ---- TinyUSB device callbacks -------------------------------------------- */
void tud_mount_cb(void)
{
    printf("usb: configured by the host (enumeration done)\n");
    led_on(LED_GREEN);
}

void tud_umount_cb(void)
{
    printf("usb: unplugged\n");
    led_off(LED_GREEN);
}

void tud_suspend_cb(bool remote_wakeup_en)
{
    (void)remote_wakeup_en;
    printf("usb: suspended (bus idle > 3 ms)\n");
}

void tud_resume_cb(void)
{
    printf("usb: resumed\n");
}

void tud_cdc_line_state_cb(uint8_t itf, bool dtr, bool rts)
{
    (void)itf;
    (void)rts;
    printf("cdc: terminal %s\n", dtr ? "opened (DTR=1)" : "closed (DTR=0)");
    if (dtr) {
        tud_cdc_write_str("\r\nlesson 24: type a line, it comes back upper-cased\r\n");
        tud_cdc_write_flush();
    }
}

void tud_cdc_line_coding_cb(uint8_t itf, const cdc_line_coding_t *coding)
{
    (void)itf;
    /* Pure bookkeeping for a virtual port: data moves at USB speed whatever the
     * host sets. A USB-to-UART bridge would reprogram its UART here. */
    printf("cdc: host set %lu baud, %u data bits\n", (unsigned long)coding->bit_rate,
           coding->data_bits);
}

static void cdc_echo_task(void)
{
    if (!tud_cdc_available()) {
        return;
    }
    char buf[64];
    uint32_t n = tud_cdc_read(buf, sizeof buf);
    for (uint32_t i = 0; i < n; i++) {
        buf[i] = (char)toupper((unsigned char)buf[i]);
        if (buf[i] == '\r') {
            tud_cdc_write_char('\n');                   /* terminals send CR for Enter */
        }
        tud_cdc_write_char(buf[i]);
    }
    tud_cdc_write_flush();
    led_toggle(LED_BLUE);
}

int main(void)
{
    clock_init_168mhz();
    board_init_leds();
    uart_init(PCLK1_HZ, 115200);
    tick_init(SYSCLK_HZ);
    printf("\nlesson 24: USB CDC, SYSCLK %lu MHz, USB clock 48 MHz\n",
           (unsigned long)(SYSCLK_HZ / 1000000u));

    usb_pins_init();
    tud_configure_dwc2_t cfg = CFG_TUD_CONFIGURE_DWC2_DEFAULT;
    cfg.vbus_sensing = true;                            /* PA9 is wired to VBUS on the Discovery */
    tud_configure(0, TUD_CFGID_DWC2, &cfg);

    tusb_rhport_init_t dev_init = {.role = TUSB_ROLE_DEVICE, .speed = TUSB_SPEED_AUTO};
    if (tusb_init(0, &dev_init)) {
        printf("usb: waiting for a host\n");
    } else {
        /* Reading the core's ID register (GSNPSID, 0x50000040) gave no DWC2 signature. */
        printf("usb: no USB controller found, stack not started\n");
        led_on(LED_RED);
    }

    for (;;) {
        tud_task();                                     /* handles queued USB events, never blocks */
        cdc_echo_task();
    }
}
