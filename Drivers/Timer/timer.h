/**
 * @file    timer.h — TIM4 编码器模式驱动
 * @note    PB6=CH1(A), PB7=CH2(B), 寄存器级配置
 */
#ifndef __TIMER_H
#define __TIMER_H

#include "stm32f1xx_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

void TIM4_Encoder_Init(void);
uint16_t TIM4_GetCount(void);
void TIM4_ResetCount(void);

#ifdef __cplusplus
}
#endif

#endif
