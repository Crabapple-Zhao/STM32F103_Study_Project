/**
 * @file    led.c — PC13 LED (低电平点亮)
 */
#include "led.h"

void LED_Init(void)
{
    __HAL_RCC_GPIOC_CLK_ENABLE();
    GPIO_InitTypeDef g = {0};
    g.Pin   = GPIO_PIN_13;
    g.Mode  = GPIO_MODE_OUTPUT_PP;
    g.Pull  = GPIO_NOPULL;
    g.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOC, &g);
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
}

void LED_On(void)    { HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET); }
void LED_Off(void)   { HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);   }
void LED_Toggle(void) { HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13); }
