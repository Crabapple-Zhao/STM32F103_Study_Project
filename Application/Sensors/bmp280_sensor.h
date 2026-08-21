/**
 * @file    bmp280_sensor.h
 * @brief   BMP280 adapter for the shared sensor runtime.
 */
#ifndef __BMP280_SENSOR_H
#define __BMP280_SENSOR_H

#include "bmp280.h"
#include "sensor_runtime.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    BMP280_Device device;
    BMP280_Reading reading;
    BMP280_Status init_status;
    uint8_t bus_acquired;
} Bmp280Sensor;

extern const SensorOps BMP280_SENSOR_OPS;

#ifdef __cplusplus
}
#endif

#endif
