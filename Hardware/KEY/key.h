/**
 * @file    key.h - key driver
 * @note    KEY1=PA0, KEY2=PC13, active high
 */
#ifndef __KEY_H
#define __KEY_H

#include "stm32f1xx_hal.h"

#define KEY1_PORT          GPIOA
#define KEY1_PIN           GPIO_PIN_0
#define KEY2_PORT          GPIOC
#define KEY2_PIN           GPIO_PIN_13
#define KEY_NUM_1          1
#define KEY_NUM_2          2
#define KEY_ACTIVE_LEVEL   GPIO_PIN_SET   /* high = pressed */

#ifdef __cplusplus
extern "C" {
#endif

void KEY_Init(void);
uint8_t KEY_Read(uint8_t key_num);

#ifdef __cplusplus
}
#endif

#endif