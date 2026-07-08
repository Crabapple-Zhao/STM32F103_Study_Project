/**
 * @file    usart.c — HAL USART1 PA9/PA10
 */
#include "usart.h"

static UART_HandleTypeDef huart1;

void USART1_Init(uint32_t baud)
{
    __HAL_RCC_USART1_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitTypeDef g = {0};
    g.Pin   = GPIO_PIN_9;
    g.Mode  = GPIO_MODE_AF_PP;
    g.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &g);

    huart1.Instance          = USART1;
    huart1.Init.BaudRate     = baud;
    huart1.Init.WordLength   = UART_WORDLENGTH_8B;
    huart1.Init.StopBits     = UART_STOPBITS_1;
    huart1.Init.Parity       = UART_PARITY_NONE;
    huart1.Init.Mode         = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl    = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;
    HAL_UART_Init(&huart1);
}

void uart_putc(char c)
{
    while (__HAL_UART_GET_FLAG(&huart1, UART_FLAG_TXE) == RESET);
    huart1.Instance->DR = (uint8_t)c;
}

void uart_puts(const char *s) { while (*s) uart_putc(*s++); }

void uart_puts_hex(uint32_t v)
{
    int i;
    uart_puts("0x");
    for (i = 28; i >= 0; i -= 4) {
        uint8_t nib = (v >> i) & 0xF;
        uart_putc(nib < 10 ? '0' + nib : 'A' + nib - 10);
    }
}
