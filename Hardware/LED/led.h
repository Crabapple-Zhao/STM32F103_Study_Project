/**
 * @file    led.h — PC13 LED
 */
#ifndef __LED_H
#define __LED_H

#include "stm32f1xx_hal.h"

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
