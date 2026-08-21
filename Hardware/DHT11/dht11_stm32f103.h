/**
 * @file    dht11_stm32f103.h
 * @brief   STM32F103 PA12 port for the DHT11 driver.
 */
#ifndef __DHT11_STM32F103_H
#define __DHT11_STM32F103_H

#include "dht11.h"

#ifdef __cplusplus
extern "C" {
#endif

const DHT11_Port *DHT11_STM32F103_GetPort(void);

#ifdef __cplusplus
}
#endif

#endif
