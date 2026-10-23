#ifndef MAIN_H
#define MAIN_H

#include "stm32f4xx_hal.h"

/* Pin labels, as CubeMX emits them from the names given in the pinout view. */
#define BUTTON_Pin        GPIO_PIN_0
#define BUTTON_GPIO_Port  GPIOA
#define LED_GREEN_Pin     GPIO_PIN_12
#define LED_ORANGE_Pin    GPIO_PIN_13
#define LED_RED_Pin       GPIO_PIN_14
#define LED_BLUE_Pin      GPIO_PIN_15

extern UART_HandleTypeDef huart2;

void Error_Handler(void);

#endif
