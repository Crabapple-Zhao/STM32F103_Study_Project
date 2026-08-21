/**
 * @file    cs100a.h
 * @brief   Platform-independent CS100A ultrasonic sensor driver.
 */
#ifndef __CS100A_H
#define __CS100A_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    CS100A_IO_BUSY = 0,
    CS100A_IO_READY,
    CS100A_IO_TIMEOUT,
    CS100A_IO_ERROR
} CS100A_IO_Status;

typedef struct {
    void *context;
    bool (*init)(void *context);
    void (*deinit)(void *context);
    bool (*start)(void *context);
    CS100A_IO_Status (*poll)(void *context, uint32_t *pulse_us);
} CS100A_Port;

typedef enum {
    CS100A_OK = 0,
    CS100A_BUSY,
    CS100A_ERR_NO_ECHO,
    CS100A_ERR_PORT,
    CS100A_ERR_NOT_INITIALIZED
} CS100A_Status;

typedef struct {
    uint32_t pulse_us;
    uint32_t distance_mm;
    CS100A_Status status;
} CS100A_Reading;

typedef struct {
    CS100A_Port port;
    uint8_t initialized;
    uint8_t measuring;
} CS100A_Device;

CS100A_Status CS100A_Init(CS100A_Device *device, const CS100A_Port *port);
void CS100A_DeInit(CS100A_Device *device);
CS100A_Status CS100A_Start(CS100A_Device *device);
CS100A_Reading CS100A_Poll(CS100A_Device *device);
const char *CS100A_StatusText(CS100A_Status status);

#ifdef __cplusplus
}
#endif

#endif
