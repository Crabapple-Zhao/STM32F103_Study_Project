/**
 * @file    spi.c — SPI1 CPOL=High, CPHA=2Edge, 18MHz
 */
#include "spi.h"

void SPI1_Init(void)
{
    __HAL_RCC_SPI1_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitTypeDef g = {0};
    g.Pin   = GPIO_PIN_5 | GPIO_PIN_7;
    g.Mode  = GPIO_MODE_AF_PP;
    g.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &g);

    SPI1->CR1 = SPI_CR1_MSTR | SPI_CR1_SSM | SPI_CR1_SSI
              | SPI_CR1_CPOL | SPI_CR1_CPHA
              | SPI_CR1_BR_0 | SPI_CR1_SPE;
}

void SPI1_WriteByte(uint8_t dat)
{
    while (!(SPI1->SR & SPI_SR_TXE));
    *((__IO uint8_t *)&SPI1->DR) = dat;
    volatile uint16_t i;
    for (i = 0; i < 20; i++);
}