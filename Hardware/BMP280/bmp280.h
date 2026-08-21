/**
 * @file    bmp280.h
 * @brief   Platform-independent BMP280 pressure sensor driver.
 */
#ifndef __BMP280_H
#define __BMP280_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    BMP280_OK = 0,
    BMP280_ERR_INVALID_ARGUMENT,
    BMP280_ERR_BUS,
    BMP280_ERR_NO_SENSOR,
    BMP280_ERR_CHIP_ID,
    BMP280_ERR_CALIBRATION,
    BMP280_ERR_CONFIG,
    BMP280_ERR_NOT_INITIALIZED,
    BMP280_ERR_DATA,
    BMP280_ERR_COMPENSATION
} BMP280_Status;

typedef bool (*BMP280_IsReadyFn)(void *context, uint8_t address7,
                                 uint32_t timeout_ms);
typedef bool (*BMP280_ReadFn)(void *context, uint8_t address7, uint8_t reg,
                              uint8_t *data, uint16_t size,
                              uint32_t timeout_ms);
typedef bool (*BMP280_WriteFn)(void *context, uint8_t address7, uint8_t reg,
                               const uint8_t *data, uint16_t size,
                               uint32_t timeout_ms);
typedef void (*BMP280_DelayFn)(void *context, uint32_t delay_ms);

typedef struct {
    void *context;
    BMP280_IsReadyFn is_ready;
    BMP280_ReadFn read;
    BMP280_WriteFn write;
    BMP280_DelayFn delay_ms;
} BMP280_Bus;

typedef struct {
    uint16_t t1;
    int16_t t2;
    int16_t t3;
    uint16_t p1;
    int16_t p2;
    int16_t p3;
    int16_t p4;
    int16_t p5;
    int16_t p6;
    int16_t p7;
    int16_t p8;
    int16_t p9;
} BMP280_Calibration;

typedef struct {
    BMP280_Bus bus;
    BMP280_Calibration calibration;
    uint8_t address;
    uint8_t chip_id;
    uint8_t initialized;
} BMP280_Device;

typedef struct {
    int32_t temperature_centi_c;
    uint32_t pressure_pa;
    BMP280_Status status;
} BMP280_Reading;

BMP280_Status BMP280_Init(BMP280_Device *device, const BMP280_Bus *bus);
void BMP280_DeInit(BMP280_Device *device);
BMP280_Reading BMP280_Read(BMP280_Device *device);
uint8_t BMP280_GetAddress(const BMP280_Device *device);
uint8_t BMP280_GetChipId(const BMP280_Device *device);
const char *BMP280_StatusText(BMP280_Status status);

#ifdef __cplusplus
}
#endif

#endif
