/**
 * @file    dma.h — DMA1 Ch3 SPI1_Tx (寄存器级)
 */
#ifndef __DMA_H
#define __DMA_H

#include "stm32f1xx_hal.h"

void DMA_Setup(void);
void DMA_SPI1_Tx(const uint8_t *buf, uint16_t len);
void DMA_SPI1_Tx16(const uint16_t *buf, uint16_t count);

#endif