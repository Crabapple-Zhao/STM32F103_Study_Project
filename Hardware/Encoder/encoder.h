/**
 * @file    encoder.h — 编码器旋钮模块
 * @note    A=PB6(TIM4_CH1), B=PB7(TIM4_CH2), SW=PB5, C=GND
 *          SW 低电平有效 (按下接通 GND)
 */
#ifndef __ENCODER_H
#define __ENCODER_H

#include "stm32f1xx_hal.h"

#define ENC_SW_PORT     GPIOB
#define ENC_SW_PIN      GPIO_PIN_5

#ifdef __cplusplus
extern "C" {
#endif

void    Encoder_Init(void);
int16_t Encoder_GetCount(void);
void    Encoder_ResetCount(void);
uint8_t Encoder_SW_Read(void);   /* 返回 1=按下, 0=释放 */

#ifdef __cplusplus
}
#endif

#endif
