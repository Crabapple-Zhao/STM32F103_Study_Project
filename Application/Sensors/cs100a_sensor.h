/**
 * @file    cs100a_sensor.h
 * @brief   CS100A adapter for the shared sensor runtime.
 */
#ifndef __CS100A_SENSOR_H
#define __CS100A_SENSOR_H

#include "cs100a.h"
#include "sensor_runtime.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    CS100A_Device device;
    CS100A_Reading reading;
    CS100A_Status init_status;
} Cs100aSensor;

extern const SensorOps CS100A_SENSOR_OPS;

#ifdef __cplusplus
}
#endif

#endif
