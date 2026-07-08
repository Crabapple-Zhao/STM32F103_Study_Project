/**
 * @file    led.h — PA11 LED (低电平点亮)
 */
#ifndef __LED_H
#define __LED_H

#include "stm32f1xx_hal.h"

#define LED_PORT          GPIOA
#define LED_PIN           GPIO_PIN_11
#define LED_ACTIVE_LEVEL  GPIO_PIN_RESET   /* low = ON */

#ifdef __cplusplus
extern "C" {
#endif

void LED_Init(void);
void LED_On(void);
void LED_Off(void);
void LED_Toggle(void);

#ifdef __cplusplus
}
#endif

#endif
