/**
 * @file    app_log.h
 * @brief   Minimal USART log helpers for boot diagnostics.
 */
#ifndef __APP_LOG_H
#define __APP_LOG_H

#include "usart.h"

#define APP_LOG_INFO(module, message) \
    uart_puts("[INFO] [" module "] " message "\r\n")

#define APP_LOG_ERROR(module, message) \
    uart_puts("[ERROR] [" module "] " message "\r\n")

#endif
