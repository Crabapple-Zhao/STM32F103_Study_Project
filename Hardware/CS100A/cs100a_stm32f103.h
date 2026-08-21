/**
 * @file    cs100a_stm32f103.h
 * @brief   STM32F103 port for CS100A on PA15/PB3 and TIM2_CH2.
 */
#ifndef __CS100A_STM32F103_H
#define __CS100A_STM32F103_H

#include "cs100a.h"

#ifdef __cplusplus
extern "C" {
#endif

const CS100A_Port *CS100A_STM32F103_GetPort(void);

#ifdef __cplusplus
}
#endif

#endif
