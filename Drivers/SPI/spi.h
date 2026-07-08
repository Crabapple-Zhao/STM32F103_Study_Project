/**
 * @file    spi.h — SPI1 寄存器级
 */
#ifndef __SPI_H
#define __SPI_H

#include "stm32f1xx_hal.h"

void SPI1_Init(void);
void SPI1_WriteByte(uint8_t dat);

#endif
