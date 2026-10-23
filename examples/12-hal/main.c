/*
 * Lesson 12: the STM32 HAL library.
 *
 * Laid out the way STM32CubeMX generates a project:
 *   main.c                 SystemClock_Config(), MX_xxx_Init(), application
 *   stm32f4xx_hal_msp.c    low-level pin/clock setup called back by HAL_xxx_Init()
 *   stm32f4xx_it.c         interrupt handlers that forward to HAL
 *   stm32f4xx_hal_conf.h   which HAL modules are compiled in
 */
#include <stdio.h>
#include <string.h>

#include "main.h"

UART_HandleTypeDef huart2;

static volatile uint32_t g_presses;
static uint8_t g_rx_byte;
static char g_line[32];
static volatile size_t g_line_len;
static volatile uint8_t g_line_ready;

static void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);

int main(void)
{
    /* Resets peripherals, sets NVIC grouping, starts SysTick at 1 kHz from the 16 MHz HSI. */
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_USART2_UART_Init();
    setvbuf(stdout, NULL, _IONBF, 0);

    uint32_t v = HAL_GetHalVersion();
    printf("\nHAL v%lu.%lu.%lu\n", (unsigned long)(v >> 24), (unsigned long)((v >> 16) & 0xFF),
           (unsigned long)((v >> 8) & 0xFF));
    printf("SYSCLK %lu Hz, HCLK %lu Hz, PCLK1 %lu Hz, PCLK2 %lu Hz\n",
           (unsigned long)HAL_RCC_GetSysClockFreq(), (unsigned long)HAL_RCC_GetHCLKFreq(),
           (unsigned long)HAL_RCC_GetPCLK1Freq(), (unsigned long)HAL_RCC_GetPCLK2Freq());

    /* Interrupt-driven reception: HAL calls HAL_UART_RxCpltCallback per byte. */
    HAL_UART_Receive_IT(&huart2, &g_rx_byte, 1);

    for (int i = 0; i < 8; i++) {
        HAL_GPIO_TogglePin(GPIOD, LED_ORANGE_Pin);
        printf("tick %d at %lu ms, orange LED %s\n", i, (unsigned long)HAL_GetTick(),
               HAL_GPIO_ReadPin(GPIOD, LED_ORANGE_Pin) == GPIO_PIN_SET ? "on" : "off");
        HAL_Delay(100);

        if (g_line_ready) {
            printf("received line: \"%s\"\n", g_line);
            g_line_len = 0;
            g_line_ready = 0;
        }
    }

    printf("button presses: %lu\n", (unsigned long)g_presses);
    for (;;) {
        __WFI();
    }
}

/* ---- Callbacks: HAL defines these as weak, the application overrides ----- */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == BUTTON_Pin) {
        g_presses++;
        HAL_GPIO_TogglePin(GPIOD, LED_BLUE_Pin);
    }
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART2) {
        if (!g_line_ready) {
            if (g_rx_byte == '\r' || g_rx_byte == '\n') {
                g_line[g_line_len] = '\0';
                g_line_ready = g_line_len > 0;
            } else if (g_line_len + 1 < sizeof g_line) {
                g_line[g_line_len++] = (char)g_rx_byte;
            }
        }
        HAL_UART_Receive_IT(&huart2, &g_rx_byte, 1);   /* re-arm for the next byte */
    }
}

int __io_putchar(int ch)
{
    uint8_t c = (uint8_t)ch;
    if (c == '\n') {
        uint8_t cr = '\r';
        HAL_UART_Transmit(&huart2, &cr, 1, HAL_MAX_DELAY);
    }
    HAL_UART_Transmit(&huart2, &c, 1, HAL_MAX_DELAY);
    return ch;
}

/* ---- Generated-style initialisation -------------------------------------- */

/* 8 MHz HSE -> PLL -> 168 MHz SYSCLK; AHB /1, APB1 /4 (42 MHz), APB2 /2 (84 MHz). */
static void SystemClock_Config(void)
{
    RCC_OscInitTypeDef osc = {0};
    RCC_ClkInitTypeDef clk = {0};

    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

    osc.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    osc.HSEState = RCC_HSE_ON;
    osc.PLL.PLLState = RCC_PLL_ON;
    osc.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    osc.PLL.PLLM = 8;
    osc.PLL.PLLN = 336;
    osc.PLL.PLLP = RCC_PLLP_DIV2;
    osc.PLL.PLLQ = 7;
    if (HAL_RCC_OscConfig(&osc) != HAL_OK) {
        Error_Handler();
    }

    clk.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    clk.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    clk.AHBCLKDivider = RCC_SYSCLK_DIV1;
    clk.APB1CLKDivider = RCC_HCLK_DIV4;
    clk.APB2CLKDivider = RCC_HCLK_DIV2;
    if (HAL_RCC_ClockConfig(&clk, FLASH_LATENCY_5) != HAL_OK) {
        Error_Handler();
    }
}

static void MX_GPIO_Init(void)
{
    GPIO_InitTypeDef gpio = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();

    HAL_GPIO_WritePin(GPIOD, LED_GREEN_Pin | LED_ORANGE_Pin | LED_RED_Pin | LED_BLUE_Pin, GPIO_PIN_RESET);

    gpio.Pin = LED_GREEN_Pin | LED_ORANGE_Pin | LED_RED_Pin | LED_BLUE_Pin;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOD, &gpio);

    gpio.Pin = BUTTON_Pin;
    gpio.Mode = GPIO_MODE_IT_RISING;
    gpio.Pull = GPIO_PULLDOWN;
    HAL_GPIO_Init(BUTTON_GPIO_Port, &gpio);

    HAL_NVIC_SetPriority(EXTI0_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(EXTI0_IRQn);
}

static void MX_USART2_UART_Init(void)
{
    huart2.Instance = USART2;
    huart2.Init.BaudRate = 115200;
    huart2.Init.WordLength = UART_WORDLENGTH_8B;
    huart2.Init.StopBits = UART_STOPBITS_1;
    huart2.Init.Parity = UART_PARITY_NONE;
    huart2.Init.Mode = UART_MODE_TX_RX;
    huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart2.Init.OverSampling = UART_OVERSAMPLING_16;
    if (HAL_UART_Init(&huart2) != HAL_OK) {
        Error_Handler();
    }
}

void Error_Handler(void)
{
    __disable_irq();
    HAL_GPIO_WritePin(GPIOD, LED_RED_Pin, GPIO_PIN_SET);
    for (;;) {
    }
}
