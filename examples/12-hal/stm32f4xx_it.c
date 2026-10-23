/*
 * Interrupt handlers. Names must match the vector table in the startup file;
 * each one hands the event to the HAL, which then calls the user callbacks.
 */
#include "main.h"

void SysTick_Handler(void)
{
    HAL_IncTick();
}

void EXTI0_IRQHandler(void)
{
    HAL_GPIO_EXTI_IRQHandler(BUTTON_Pin);
}

void USART2_IRQHandler(void)
{
    HAL_UART_IRQHandler(&huart2);
}

void HardFault_Handler(void)
{
    for (;;) {
    }
}
