/**
 * @file    dht11.h
 * @brief   Platform-independent DHT11 protocol driver.
 */
#ifndef __DHT11_H
#define __DHT11_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    DHT11_OK = 0,
    DHT11_ERR_INVALID_ARGUMENT,
    DHT11_ERR_PORT,
    DHT11_ERR_NOT_INITIALIZED,
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

typedef struct {
    void *context;
    bool (*init)(void *context);
    void (*deinit)(void *context);
    void (*set_output)(void *context);
    void (*set_input)(void *context);
    void (*write)(void *context, bool high);
    void (*delay_us)(void *context, uint32_t delay_us);
    void (*delay_ms)(void *context, uint32_t delay_ms);
    uint32_t (*critical_enter)(void *context);
    void (*critical_exit)(void *context, uint32_t state);
    bool (*wait_while)(void *context, bool high, uint32_t timeout_us,
                       uint32_t *duration_us);
} DHT11_Port;

typedef struct {
    DHT11_Port port;
    uint8_t initialized;
} DHT11_Device;

DHT11_Status DHT11_Init(DHT11_Device *device, const DHT11_Port *port);
void DHT11_DeInit(DHT11_Device *device);
DHT11_Reading DHT11_Read(DHT11_Device *device);
const char *DHT11_StatusText(DHT11_Status status);

#ifdef __cplusplus
}
#endif

#endif
