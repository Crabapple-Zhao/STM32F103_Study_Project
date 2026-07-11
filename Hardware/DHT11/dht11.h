/**
 * @file dht11.h
 * @brief DHT11 single-wire temperature/humidity sensor on PA12.
 */
#ifndef __DHT11_H
#define __DHT11_H

#include "stm32f1xx_hal.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    DHT11_OK = 0,
    DHT11_ERR_RESPONSE_HIGH,
    DHT11_ERR_RESPONSE_LOW,
    DHT11_ERR_RESPONSE_RELEASE,
    DHT11_ERR_BIT_LOW,
    DHT11_ERR_BIT_HIGH,
    DHT11_ERR_CHECKSUM
} DHT11_Status;

typedef struct {
    uint8_t humidity;
    uint8_t temperature;
    DHT11_Status status;
} DHT11_Reading;

void DHT11_Init(void);
void DHT11_DeInit(void);
DHT11_Reading DHT11_Read(void);
uint8_t DHT11_GetLastReading(DHT11_Reading *reading);
const char *DHT11_StatusText(DHT11_Status status);

#ifdef __cplusplus
}
#endif

#endif
