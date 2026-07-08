/**
 * @file    led.c — PA11 LED (低电平点亮)
 */
#include "led.h"

void LED_Init(void)
{
    __HAL_RCC_GPIOA_CLK_ENABLE();
    GPIO_InitTypeDef g = {0};
    g.Pin   = LED_PIN;
    g.Mode  = GPIO_MODE_OUTPUT_PP;
    g.Pull  = GPIO_NOPULL;
    g.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LED_PORT, &g);
    HAL_GPIO_WritePin(LED_PORT, LED_PIN, (GPIO_PinState)!LED_ACTIVE_LEVEL);
}

void LED_On(void)    { HAL_GPIO_WritePin(LED_PORT, LED_PIN, LED_ACTIVE_LEVEL); }
void LED_Off(void)   { HAL_GPIO_WritePin(LED_PORT, LED_PIN, (GPIO_PinState)!LED_ACTIVE_LEVEL); }
void LED_Toggle(void) { HAL_GPIO_TogglePin(LED_PORT, LED_PIN); }
