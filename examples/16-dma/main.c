/*
 * Lesson 16: DMA.
 *
 *   Part 1  DMA2 stream 1, memory -> memory: copy a buffer without the CPU.
 *   Part 2  DMA1 stream 6 channel 4, memory -> USART2_TX: the UART pulls one
 *           byte per request while the CPU keeps working.
 *
 * Only DMA2 can do memory-to-memory transfers. Stream/channel pairs for each
 * peripheral request are fixed: see the DMA1/DMA2 request mapping tables in RM0090.
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
    DMA_Stream_TypeDef *s = DMA2_Stream1;

    s->CR &= ~DMA_SxCR_EN;
    while (s->CR & DMA_SxCR_EN) {          /* wait until the stream really stops */
    }
    DMA2->LIFCR = DMA_LIFCR_CTCIF1 | DMA_LIFCR_CHTIF1 | DMA_LIFCR_CTEIF1 |
                  DMA_LIFCR_CDMEIF1 | DMA_LIFCR_CFEIF1;

    s->PAR = (uint32_t)src;                /* in M2M mode PAR is the source */
    s->M0AR = (uint32_t)dst;
    s->NDTR = words;                       /* number of data items, not bytes */
    s->FCR = DMA_SxFCR_DMDIS | DMA_SxFCR_FTH;   /* M2M needs the FIFO */
    s->CR = (0u << DMA_SxCR_CHSEL_Pos)
          | DMA_SxCR_DIR_1                 /* 10: memory to memory */
          | DMA_SxCR_PINC | DMA_SxCR_MINC  /* advance both addresses */
          | DMA_SxCR_PSIZE_1 | DMA_SxCR_MSIZE_1   /* 32-bit items */
          | DMA_SxCR_PL_1                  /* priority high */
          | DMA_SxCR_TCIE;                 /* interrupt on transfer complete */
    s->CR |= DMA_SxCR_EN;
}

void DMA2_Stream1_IRQHandler(void)
{
    if (DMA2->LISR & DMA_LISR_TCIF1) {
        DMA2->LIFCR = DMA_LIFCR_CTCIF1;
        g_m2m_done = 1;
    }
}

/* ---- Part 2: memory to USART2 ------------------------------------------- */
static void uart_dma_init(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_DMA1EN;
    USART2->CR3 |= USART_CR3_DMAT;         /* USART asks DMA for each TX byte */
    NVIC_SetPriority(DMA1_Stream6_IRQn, 5);
    NVIC_EnableIRQ(DMA1_Stream6_IRQn);
}

static void uart_dma_send(const char *buf, uint32_t len)
{
    DMA_Stream_TypeDef *s = DMA1_Stream6;

    s->CR &= ~DMA_SxCR_EN;
    while (s->CR & DMA_SxCR_EN) {
    }
    DMA1->HIFCR = DMA_HIFCR_CTCIF6 | DMA_HIFCR_CHTIF6 | DMA_HIFCR_CTEIF6 |
                  DMA_HIFCR_CDMEIF6 | DMA_HIFCR_CFEIF6;

    s->PAR = (uint32_t)&USART2->DR;        /* fixed peripheral address */
    s->M0AR = (uint32_t)buf;
    s->NDTR = len;
    s->FCR = 0;                            /* direct mode: one byte per request */
    s->CR = (4u << DMA_SxCR_CHSEL_Pos)     /* channel 4 = USART2_TX */
          | DMA_SxCR_DIR_0                 /* 01: memory to peripheral */
          | DMA_SxCR_MINC                  /* walk the buffer, keep PAR fixed */
          | DMA_SxCR_TCIE;                 /* 8-bit sizes are the 00 default */

    g_tx_done = 0;
    USART2->SR &= ~USART_SR_TC;
    s->CR |= DMA_SxCR_EN;
}

void DMA1_Stream6_IRQHandler(void)
{
    if (DMA1->HISR & DMA_HISR_TCIF6) {
        DMA1->HIFCR = DMA_HIFCR_CTCIF6;
        g_tx_done = 1;
    }
}

int main(void)
{
    board_init_leds();
    uart_init(HSI_HZ, 115200);
    tick_init(HSI_HZ);

    printf("\nlesson 16: DMA\n");

    /* Part 1 */
    RCC->AHB1ENR |= RCC_AHB1ENR_DMA2EN;
    NVIC_SetPriority(DMA2_Stream1_IRQn, 5);
    NVIC_EnableIRQ(DMA2_Stream1_IRQn);

    for (uint32_t i = 0; i < WORDS; i++) {
        g_src[i] = i * 0x01010101u;
    }
    memset(g_dst, 0, sizeof g_dst);

    dma_m2m_copy(g_src, g_dst, WORDS);
    while (!g_m2m_done) {
        __WFI();
    }
    printf("M2M: copied %u words, NDTR now %lu, buffers %s\n", (unsigned)WORDS,
           (unsigned long)DMA2_Stream1->NDTR, memcmp(g_src, g_dst, sizeof g_src) == 0 ? "match" : "DIFFER");
    printf("     g_dst[1] = 0x%08lx, g_dst[255] = 0x%08lx\n", (unsigned long)g_dst[1], (unsigned long)g_dst[255]);

    /* Part 2 */
    uart_dma_init();
    static const char msg[] =
        "DMA: this whole line is pushed into USART2 by DMA1 stream 6,\r\n"
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

    led_on(LED_GREEN);
    for (;;) {
        __WFI();
    }
}
