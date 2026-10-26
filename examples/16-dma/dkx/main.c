/*
 * Lesson 16, Dikuoxing STM32F103C8T6 version: DMA.
 *
 *   Part 1  DMA1 channel 1, memory -> memory: copy a buffer without the CPU.
 *   Part 2  DMA1 channel 4, memory -> USART1_TX: the UART pulls one byte per
 *           request while the CPU keeps working.
 *
 * The F1 DMA is simpler than the F4's: no streams, no FIFO, no CHSEL. Each
 * peripheral request is hard-wired to one channel (RM0008, "DMA1 request
 * mapping"), and any channel can copy memory to memory with MEM2MEM.
 */
#include <stdio.h>
#include <string.h>

#include "board.h"
#include "tick.h"
#include "uart.h"

#define WORDS 256u

static uint32_t g_src[WORDS];
static uint32_t g_dst[WORDS];

static volatile uint32_t g_m2m_done;
static volatile uint32_t g_tx_done;

/* ---- Part 1: memory to memory ------------------------------------------- */
static void dma_m2m_copy(const uint32_t *src, uint32_t *dst, uint32_t words)
{
    DMA_Channel_TypeDef *ch = DMA1_Channel1;

    ch->CCR &= ~DMA_CCR_EN;                /* CPAR/CMAR/CNDTR are writable only while off */
    DMA1->IFCR = DMA_IFCR_CGIF1;           /* clears TC, HT and TE of channel 1 too */

    ch->CPAR = (uint32_t)src;              /* DIR = 0: "peripheral" side is the source */
    ch->CMAR = (uint32_t)dst;
    ch->CNDTR = words;                     /* number of data items, not bytes */
    ch->CCR = DMA_CCR_MEM2MEM              /* no request needed: run as fast as possible */
            | DMA_CCR_PINC | DMA_CCR_MINC  /* advance both addresses */
            | DMA_CCR_PSIZE_1 | DMA_CCR_MSIZE_1   /* 32-bit items */
            | DMA_CCR_PL_1                 /* priority high */
            | DMA_CCR_TCIE;                /* interrupt on transfer complete */
    ch->CCR |= DMA_CCR_EN;
}

void DMA1_Channel1_IRQHandler(void)
{
    if (DMA1->ISR & DMA_ISR_TCIF1) {
        DMA1->IFCR = DMA_IFCR_CTCIF1;
        g_m2m_done = 1;
    }
}

/* ---- Part 2: memory to USART1 ------------------------------------------- */
static void uart_dma_init(void)
{
    USART1->CR3 |= USART_CR3_DMAT;         /* USART asks DMA for each TX byte */
    NVIC_SetPriority(DMA1_Channel4_IRQn, 5);
    NVIC_EnableIRQ(DMA1_Channel4_IRQn);
}

static void uart_dma_send(const char *buf, uint32_t len)
{
    DMA_Channel_TypeDef *ch = DMA1_Channel4;   /* USART1_TX is wired to channel 4 */

    ch->CCR &= ~DMA_CCR_EN;
    DMA1->IFCR = DMA_IFCR_CGIF4;

    ch->CPAR = (uint32_t)&USART1->DR;      /* fixed peripheral address */
    ch->CMAR = (uint32_t)buf;
    ch->CNDTR = len;
    ch->CCR = DMA_CCR_DIR                  /* 1: memory to peripheral */
            | DMA_CCR_MINC                 /* walk the buffer, keep CPAR fixed */
            | DMA_CCR_TCIE;                /* 8-bit sizes are the 00 default */

    g_tx_done = 0;
    USART1->SR &= ~USART_SR_TC;
    ch->CCR |= DMA_CCR_EN;
}

void DMA1_Channel4_IRQHandler(void)
{
    if (DMA1->ISR & DMA_ISR_TCIF4) {
        DMA1->IFCR = DMA_IFCR_CTCIF4;
        g_tx_done = 1;
    }
}

int main(void)
{
    board_init_led();
    uart_init(HSI_HZ, 115200);
    tick_init(HSI_HZ);

    printf("\nlesson 16: DMA\n");

    /* Part 1 */
    RCC->AHBENR |= RCC_AHBENR_DMA1EN;
    NVIC_SetPriority(DMA1_Channel1_IRQn, 5);
    NVIC_EnableIRQ(DMA1_Channel1_IRQn);

    for (uint32_t i = 0; i < WORDS; i++) {
        g_src[i] = i * 0x01010101u;
    }
    memset(g_dst, 0, sizeof g_dst);

    dma_m2m_copy(g_src, g_dst, WORDS);
    while (!g_m2m_done) {
        __WFI();
    }
    printf("M2M: copied %u words, CNDTR now %lu, buffers %s\n", (unsigned)WORDS,
           (unsigned long)DMA1_Channel1->CNDTR, memcmp(g_src, g_dst, sizeof g_src) == 0 ? "match" : "DIFFER");
    printf("     g_dst[1] = 0x%08lx, g_dst[255] = 0x%08lx\n", (unsigned long)g_dst[1], (unsigned long)g_dst[255]);

    /* Part 2 */
    uart_dma_init();
    static const char msg[] =
        "DMA: this whole line is pushed into USART1 by DMA1 channel 4,\r\n"
        "     one byte per TXE request, while the CPU counts below.\r\n";
    uart_flush();

    uint32_t t0 = millis();
    uart_dma_send(msg, sizeof msg - 1);
    uint32_t spins = 0;
    while (!g_tx_done) {
        spins++;                            /* the CPU is free during the transfer */
    }
    uart_flush();
    uint32_t t1 = millis();

    unsigned bytes = (unsigned)(sizeof msg - 1);
    unsigned tenth_ms = (bytes * 10u * 10000u + 57600u) / 115200u;   /* 10 bits per byte */
    printf("TX: %u bytes done after %lu ms (on silicon 115200 baud needs %u.%u ms), "
           "CPU looped %lu times meanwhile\n", bytes, (unsigned long)(t1 - t0),
           tenth_ms / 10u, tenth_ms % 10u, (unsigned long)spins);

    led_on();
    for (;;) {
        __WFI();
    }
}
