/*
 * Lesson 18, Dikuoxing STM32F103C8T6 version: SPI bus and an external NOR
 * flash (W25Q64 in the LCSC beginner manual; Renode simulates an MT25Q, which
 * speaks the same basic command set).
 *
 *   PA5 SCK   PA6 MISO   PA7 MOSI   (SPI1)     PA4 chip select (GPIO)
 *
 * Every SPI transfer is full duplex: one byte goes out on MOSI while one byte
 * comes in on MISO. To read, the master sends dummy bytes.
 */
#include <stdio.h>
#include <string.h>

#include "board.h"
#include "tick.h"
#include "uart.h"

#define CS_PIN 4u

/* Common SPI NOR commands (MT25Q, W25Q, MX25... all agree on these) */
#define CMD_READ_ID       0x9Fu
#define CMD_READ_STATUS   0x05u
#define CMD_WRITE_ENABLE  0x06u
#define CMD_PAGE_PROGRAM  0x02u
#define CMD_READ          0x03u
#define CMD_ERASE_4K      0x20u

#define STATUS_WIP 0x01u   /* write in progress */
#define STATUS_WEL 0x02u   /* write enable latch */

/* ---- SPI1 driver --------------------------------------------------------- */
static void spi1_init(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_SPI1EN;

    gpio_config(GPIOA, 5, GPIO_CFG_AF_PP_50MHZ);    /* SCK and MOSI are driven by SPI1 */
    gpio_config(GPIOA, 7, GPIO_CFG_AF_PP_50MHZ);
    gpio_config(GPIOA, 6, GPIO_CFG_IN_FLOATING);    /* MISO is a plain input */
    GPIOA->BSRR = 1u << CS_PIN;              /* deselect before driving the pin */
    gpio_config(GPIOA, CS_PIN, GPIO_CFG_OUT_PP_2MHZ);

    /* Master, mode 0 (CPOL=0, CPHA=0), MSB first, 8-bit, f_PCLK2/4 = 2 MHz.
     * SSM+SSI: NSS is managed in software, so the hardware NSS pin is unused. */
    SPI1->CR1 = SPI_CR1_MSTR | SPI_CR1_BR_0 | SPI_CR1_SSM | SPI_CR1_SSI;
    SPI1->CR1 |= SPI_CR1_SPE;
}

static uint8_t spi1_transfer(uint8_t out)
{
    while ((SPI1->SR & SPI_SR_TXE) == 0) {
    }
    *(volatile uint8_t *)&SPI1->DR = out;    /* 8-bit access keeps the frame 8 bits */
    while ((SPI1->SR & SPI_SR_RXNE) == 0) {
    }
    return *(volatile uint8_t *)&SPI1->DR;
}

static void cs_low(void) { GPIOA->BRR = 1u << CS_PIN; }

static void cs_high(void)
{
    while (SPI1->SR & SPI_SR_BSY) {          /* last bit must leave before CS rises */
    }
    GPIOA->BSRR = 1u << CS_PIN;
}

/* ---- Flash commands ------------------------------------------------------ */
static void flash_cmd_addr(uint8_t cmd, uint32_t addr)
{
    spi1_transfer(cmd);
    spi1_transfer((uint8_t)(addr >> 16));
    spi1_transfer((uint8_t)(addr >> 8));
    spi1_transfer((uint8_t)addr);
}

static uint8_t flash_status(void)
{
    cs_low();
    spi1_transfer(CMD_READ_STATUS);
    uint8_t s = spi1_transfer(0xFF);
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
    spi1_transfer(CMD_WRITE_ENABLE);
    cs_high();
}

static void flash_read(uint32_t addr, uint8_t *buf, size_t len)
{
    cs_low();
    flash_cmd_addr(CMD_READ, addr);
    for (size_t i = 0; i < len; i++) {
        buf[i] = spi1_transfer(0xFF);        /* dummy byte out, data byte in */
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
        spi1_transfer(data[i]);
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
    board_init_led();
    uart_init(HSI_HZ, 115200);
    tick_init(HSI_HZ);
    spi1_init();

    printf("\nlesson 18: SPI1 master, mode 0, %lu Hz\n", (unsigned long)(HSI_HZ / 4u));

    uint8_t id[3];
    cs_low();
    spi1_transfer(CMD_READ_ID);
    for (int i = 0; i < 3; i++) {
        id[i] = spi1_transfer(0xFF);
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

    led_on();
    for (;;) {
        __WFI();
    }
}
