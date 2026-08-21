/**
 * @file    dht11_sensor.c
 * @brief   DHT11 adapter for the shared sensor runtime.
 */
#include "dht11_sensor.h"
#include "dht11_stm32f103.h"

static SensorResult map_status(DHT11_Status status)
{
    switch (status) {
        case DHT11_OK:
            return SENSOR_RESULT_OK;
        case DHT11_ERR_RESPONSE_HIGH:
        case DHT11_ERR_RESPONSE_LOW:
        case DHT11_ERR_RESPONSE_RELEASE:
            return SENSOR_RESULT_NO_SENSOR;
        case DHT11_ERR_BIT_LOW:
        case DHT11_ERR_BIT_HIGH:
        case DHT11_ERR_CHECKSUM:
            return SENSOR_RESULT_DATA_ERROR;
        case DHT11_ERR_INVALID_ARGUMENT:
        case DHT11_ERR_PORT:
        case DHT11_ERR_NOT_INITIALIZED:
        default:
            return SENSOR_RESULT_HARDWARE_ERROR;
    }
}

static SensorResult sensor_connect(void *context)
{
    Dht11Sensor *sensor = (Dht11Sensor *)context;
    sensor->reading.humidity = 0U;
    sensor->reading.temperature = 0U;
    sensor->reading.status = DHT11_ERR_NOT_INITIALIZED;
    sensor->init_status = DHT11_Init(&sensor->device,
                                     DHT11_STM32F103_GetPort());
    return map_status(sensor->init_status);
}

static void sensor_disconnect(void *context)
{
    Dht11Sensor *sensor = (Dht11Sensor *)context;
    DHT11_DeInit(&sensor->device);
}

static SensorResult sensor_start(void *context)
{
    Dht11Sensor *sensor = (Dht11Sensor *)context;
    sensor->reading = DHT11_Read(&sensor->device);
    return map_status(sensor->reading.status);
}

static SensorResult sensor_poll(void *context)
{
    (void)context;
    return SENSOR_RESULT_HARDWARE_ERROR;
}

const SensorOps DHT11_SENSOR_OPS = {
    sensor_connect,
    sensor_disconnect,
    sensor_start,
    sensor_poll
};
