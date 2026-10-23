/*
 * Lesson 18: SPI bus and an external NOR flash (MT25Q / W25Qxx command set).
 *
 *   PB13 SCK   PB14 MISO   PB15 MOSI   (SPI2, AF5)     PB12 chip select (GPIO)
 *
 * Every SPI transfer is full duplex: one byte goes out on MOSI while one byte
 * comes in on MISO. To read, the master sends dummy bytes.
 */
#include <stdio.h>
#include <string.h>

#include "board.h"
#include "tick.h"
#include "uart.h"

#define CS_PIN 12u

/* Common SPI NOR commands (MT25Q, W25Q, MX25... all agree on these) */
#define CMD_READ_ID       0x9Fu
#define CMD_READ_STATUS   0x05u
#define CMD_WRITE_ENABLE  0x06u
#define CMD_PAGE_PROGRAM  0x02u
#define CMD_READ          0x03u
#define CMD_ERASE_4K      0x20u

#define STATUS_WIP 0x01u   /* write in progress */
#define STATUS_WEL 0x02u   /* write enable latch */

/* ---- SPI2 driver --------------------------------------------------------- */
static void spi2_init(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;
    RCC->APB1ENR |= RCC_APB1ENR_SPI2EN;

    gpio_set_af(GPIOB, 13, 5);
    gpio_set_af(GPIOB, 14, 5);
    gpio_set_af(GPIOB, 15, 5);
    GPIOB->BSRR = 1u << CS_PIN;              /* deselect before driving the pin */
    gpio_set_mode(GPIOB, CS_PIN, GPIO_MODE_OUTPUT);

    /* Master, mode 0 (CPOL=0, CPHA=0), MSB first, 8-bit, f_PCLK/4 = 4 MHz.
     * SSM+SSI: NSS is managed in software, so the hardware NSS pin is unused. */
    SPI2->CR1 = SPI_CR1_MSTR | SPI_CR1_BR_0 | SPI_CR1_SSM | SPI_CR1_SSI;
    SPI2->CR1 |= SPI_CR1_SPE;
}

static uint8_t spi2_transfer(uint8_t out)
{
    while ((SPI2->SR & SPI_SR_TXE) == 0) {
    }
    *(volatile uint8_t *)&SPI2->DR = out;    /* 8-bit access keeps the frame 8 bits */
    while ((SPI2->SR & SPI_SR_RXNE) == 0) {
    }
    return *(volatile uint8_t *)&SPI2->DR;
}

static void cs_low(void) { GPIOB->BSRR = 1u << (CS_PIN + 16u); }

static void cs_high(void)
{
    while (SPI2->SR & SPI_SR_BSY) {          /* last bit must leave before CS rises */
    }
    GPIOB->BSRR = 1u << CS_PIN;
}

/* ---- Flash commands ------------------------------------------------------ */
static void flash_cmd_addr(uint8_t cmd, uint32_t addr)
{
    spi2_transfer(cmd);
    spi2_transfer((uint8_t)(addr >> 16));
    spi2_transfer((uint8_t)(addr >> 8));
    spi2_transfer((uint8_t)addr);
}

static uint8_t flash_status(void)
{
    cs_low();
    spi2_transfer(CMD_READ_STATUS);
    uint8_t s = spi2_transfer(0xFF);
    cs_high();
    return s;
}

static uint32_t flash_wait_ready(void)
{
    uint32_t polls = 0;
    while (flash_status() & STATUS_WIP) {
        polls++;
    }
    return polls;
}

static void flash_write_enable(void)
{
    cs_low();
    spi2_transfer(CMD_WRITE_ENABLE);
    cs_high();
}

static void flash_read(uint32_t addr, uint8_t *buf, size_t len)
{
    cs_low();
    flash_cmd_addr(CMD_READ, addr);
    for (size_t i = 0; i < len; i++) {
        buf[i] = spi2_transfer(0xFF);        /* dummy byte out, data byte in */
    }
    cs_high();
}

static void flash_erase_4k(uint32_t addr)
{
    flash_write_enable();
    cs_low();
    flash_cmd_addr(CMD_ERASE_4K, addr);
    cs_high();                               /* erase starts when CS goes high */
    flash_wait_ready();
}

/* A page program must not cross a 256-byte page boundary: it would wrap. */
static void flash_program(uint32_t addr, const uint8_t *data, size_t len)
{
    flash_write_enable();
    cs_low();
    flash_cmd_addr(CMD_PAGE_PROGRAM, addr);
    for (size_t i = 0; i < len; i++) {
        spi2_transfer(data[i]);
    }
    cs_high();
    flash_wait_ready();
}

static void dump(const char *label, const uint8_t *buf, size_t len)
{
    printf("%s", label);
    for (size_t i = 0; i < len; i++) {
        printf(" %02x", buf[i]);
    }
    printf("\n");
}

int main(void)
{
    board_init_leds();
    uart_init(HSI_HZ, 115200);
    tick_init(HSI_HZ);
    spi2_init();

    printf("\nlesson 18: SPI2 master, mode 0, %lu Hz\n", (unsigned long)(HSI_HZ / 4u));

    uint8_t id[3];
    cs_low();
    spi2_transfer(CMD_READ_ID);
    for (int i = 0; i < 3; i++) {
        id[i] = spi2_transfer(0xFF);
    }
    cs_high();
    printf("JEDEC ID: manufacturer 0x%02x, type 0x%02x, capacity 0x%02x (%lu MB)\n",
           id[0], id[1], id[2], (unsigned long)((1ul << id[2]) >> 20));

    printf("status: 0x%02x\n", flash_status());
    flash_write_enable();
    printf("after WRITE ENABLE: 0x%02x (WEL=%d)\n", flash_status(), !!(flash_status() & STATUS_WEL));

    const uint32_t addr = 0x001000;
    uint8_t buf[16];

    flash_erase_4k(addr);
    flash_read(addr, buf, sizeof buf);
    dump("after erase:  ", buf, sizeof buf);

    static const char text[] = "Hello, SPI!";
    flash_program(addr, (const uint8_t *)text, sizeof text);
    flash_read(addr, buf, sizeof buf);
    dump("after program:", buf, sizeof buf);
    printf("as text: \"%s\"\n", (const char *)buf);

    /* NOR flash programming can only turn 1 bits into 0 bits. */
    uint8_t a = 0xF0, b = 0x3C;
    flash_program(addr + 0x100, &a, 1);
    flash_program(addr + 0x100, &b, 1);
    flash_read(addr + 0x100, buf, 1);
    printf("program 0x%02x then 0x%02x without erase -> read 0x%02x "
           "(real NOR flash: 0x%02x & 0x%02x = 0x%02x)\n", a, b, buf[0], a, b, a & b);

    led_on(LED_GREEN);
    for (;;) {
        __WFI();
    }
}
