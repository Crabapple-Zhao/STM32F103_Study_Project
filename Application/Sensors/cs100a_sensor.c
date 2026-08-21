/**
 * @file    cs100a_sensor.c
 * @brief   CS100A adapter for the shared sensor runtime.
 */
#include "cs100a_sensor.h"
#include "cs100a_stm32f103.h"

static SensorResult map_status(CS100A_Status status)
{
    switch (status) {
        case CS100A_OK:                  return SENSOR_RESULT_OK;
        case CS100A_BUSY:                return SENSOR_RESULT_BUSY;
        case CS100A_ERR_NO_ECHO:         return SENSOR_RESULT_NO_DATA;
        case CS100A_ERR_PORT:
        case CS100A_ERR_NOT_INITIALIZED:
        default:                         return SENSOR_RESULT_HARDWARE_ERROR;
    }
}

static SensorResult sensor_connect(void *context)
{
    Cs100aSensor *sensor = (Cs100aSensor *)context;
    sensor->reading.pulse_us = 0U;
    sensor->reading.distance_mm = 0U;
    sensor->reading.status = CS100A_BUSY;
    sensor->init_status = CS100A_Init(&sensor->device,
                                      CS100A_STM32F103_GetPort());
    return map_status(sensor->init_status);
}

static void sensor_disconnect(void *context)
{
    Cs100aSensor *sensor = (Cs100aSensor *)context;
    CS100A_DeInit(&sensor->device);
}

static SensorResult sensor_start(void *context)
{
    Cs100aSensor *sensor = (Cs100aSensor *)context;
    return map_status(CS100A_Start(&sensor->device));
}

static SensorResult sensor_poll(void *context)
{
    Cs100aSensor *sensor = (Cs100aSensor *)context;
    CS100A_Reading reading = CS100A_Poll(&sensor->device);
    if (reading.status != CS100A_BUSY) {
        sensor->reading = reading;
    }
    return map_status(reading.status);
}

const SensorOps CS100A_SENSOR_OPS = {
    sensor_connect,
    sensor_disconnect,
    sensor_start,
    sensor_poll
};
