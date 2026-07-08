/**
 * @file    usart.h — HAL USART1
 */
#ifndef __USART_H
#define __USART_H

#include "stm32f1xx_hal.h"

void USART1_Init(uint32_t baud);
void uart_putc(char c);
void uart_puts(const char *s);
void uart_puts_hex(uint32_t v);

#endif
