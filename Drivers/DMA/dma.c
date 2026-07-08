/**
 * @file    dma.c — 寄存器级 DMA1 Ch3 (SPI1_Tx)
 */
#include "dma.h"

void DMA_Setup(void)
{
    __HAL_RCC_DMA1_CLK_ENABLE();
}

void DMA_SPI1_Tx(const uint8_t *buf, uint16_t len)
{
    if (len == 0) return;
    DMA1_Channel3->CCR   = 0;
    while (DMA1_Channel3->CCR & DMA_CCR_EN);
    DMA1->IFCR          = DMA_IFCR_CTCIF3;
    DMA1_Channel3->CPAR  = (uint32_t)&SPI1->DR;
    DMA1_Channel3->CMAR  = (uint32_t)buf;
    DMA1_Channel3->CNDTR = len;
    DMA1_Channel3->CCR   = DMA_CCR_DIR | DMA_CCR_MINC | DMA_CCR_EN;
    SPI1->CR2 |= SPI_CR2_TXDMAEN;
    uint32_t timeout = 1000000;
    while (!(DMA1->ISR & DMA_ISR_TCIF3) && --timeout);
    if (timeout == 0) return;
    DMA1->IFCR = DMA_IFCR_CTCIF3;
    SPI1->CR2 &= ~SPI_CR2_TXDMAEN;
}

void DMA_SPI1_Tx16(const uint16_t *buf, uint16_t count)
{
    if (count == 0) return;
    DMA1_Channel3->CCR   = 0;
    uint32_t wt = 10000;
    while (DMA1_Channel3->CCR & DMA_CCR_EN && --wt);
    if (wt == 0) return;
    DMA1->IFCR          = DMA_IFCR_CTCIF3;
    DMA1_Channel3->CPAR  = (uint32_t)&SPI1->DR;
    DMA1_Channel3->CMAR  = (uint32_t)buf;
    DMA1_Channel3->CNDTR = count;
    DMA1_Channel3->CCR   = DMA_CCR_DIR | DMA_CCR_PSIZE_0 | DMA_CCR_MSIZE_0 | DMA_CCR_EN;
    SPI1->CR2 |= SPI_CR2_TXDMAEN;
    uint32_t timeout = 1000000;
    while (!(DMA1->ISR & DMA_ISR_TCIF3) && --timeout);
    DMA1->IFCR = DMA_IFCR_CTCIF3;
    SPI1->CR2 &= ~SPI_CR2_TXDMAEN;
}