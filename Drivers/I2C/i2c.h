/**
 * @file    i2c.h
 * @brief   I2C2 polling driver on PB10/PB11.
 */
#ifndef __I2C_H
#define __I2C_H

#include "stm32f1xx_hal.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

bool I2C2_BusInit(void);
void I2C2_BusDeInit(void);
bool I2C2_BusIsReady(uint8_t address7, uint32_t timeout_ms);
bool I2C2_BusMemRead(uint8_t address7, uint8_t reg, uint8_t *data,
                     uint16_t size, uint32_t timeout_ms);
bool I2C2_BusMemWrite(uint8_t address7, uint8_t reg, const uint8_t *data,
                      uint16_t size, uint32_t timeout_ms);

#ifdef __cplusplus
}
#endif

#endif
