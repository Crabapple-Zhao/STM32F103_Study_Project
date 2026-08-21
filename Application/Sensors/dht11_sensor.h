/**
 * @file    dht11_sensor.h
 * @brief   DHT11 adapter for the shared sensor runtime.
 */
#ifndef __DHT11_SENSOR_H
#define __DHT11_SENSOR_H

#include "dht11.h"
#include "sensor_runtime.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    DHT11_Device device;
    DHT11_Reading reading;
    DHT11_Status init_status;
} Dht11Sensor;

extern const SensorOps DHT11_SENSOR_OPS;

#ifdef __cplusplus
}
#endif

#endif
