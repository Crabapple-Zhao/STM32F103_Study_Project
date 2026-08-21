/**
 * @file    bmp280_sensor.c
 * @brief   BMP280 adapter for the shared sensor runtime.
 */
#include "bmp280_sensor.h"
#include "i2c.h"
#include "stm32f1xx_hal.h"

static bool bus_is_ready(void *context, uint8_t address7, uint32_t timeout_ms)
{
    (void)context;
    return I2C2_BusIsReady(address7, timeout_ms);
}

static bool bus_read(void *context, uint8_t address7, uint8_t reg,
                     uint8_t *data, uint16_t size, uint32_t timeout_ms)
{
    (void)context;
    return I2C2_BusMemRead(address7, reg, data, size, timeout_ms);
}

static bool bus_write(void *context, uint8_t address7, uint8_t reg,
                      const uint8_t *data, uint16_t size, uint32_t timeout_ms)
{
    (void)context;
    return I2C2_BusMemWrite(address7, reg, data, size, timeout_ms);
}

static void bus_delay(void *context, uint32_t delay_ms)
{
    (void)context;
    HAL_Delay(delay_ms);
}

static const BMP280_Bus bmp280_bus = {
    0,
    bus_is_ready,
    bus_read,
    bus_write,
    bus_delay
};

static SensorResult map_status(BMP280_Status status)
{
    switch (status) {
        case BMP280_OK:
            return SENSOR_RESULT_OK;
        case BMP280_ERR_BUS:
        case BMP280_ERR_NO_SENSOR:
            return SENSOR_RESULT_NO_SENSOR;
        case BMP280_ERR_CHIP_ID:
        case BMP280_ERR_CALIBRATION:
        case BMP280_ERR_CONFIG:
        case BMP280_ERR_DATA:
        case BMP280_ERR_COMPENSATION:
            return SENSOR_RESULT_DATA_ERROR;
        case BMP280_ERR_INVALID_ARGUMENT:
        case BMP280_ERR_NOT_INITIALIZED:
        default:
            return SENSOR_RESULT_HARDWARE_ERROR;
    }
}

static void release_bus(Bmp280Sensor *sensor)
{
    if (sensor->bus_acquired == 0U) return;
    BMP280_DeInit(&sensor->device);
    I2C2_BusRelease();
    sensor->bus_acquired = 0U;
}

static SensorResult sensor_connect(void *context)
{
    Bmp280Sensor *sensor = (Bmp280Sensor *)context;
    SensorResult result;

    sensor->reading.status = BMP280_ERR_NOT_INITIALIZED;
    sensor->init_status = BMP280_ERR_NOT_INITIALIZED;
    if (!I2C2_BusAcquire()) {
        sensor->init_status = BMP280_ERR_BUS;
        return SENSOR_RESULT_HARDWARE_ERROR;
    }
    sensor->bus_acquired = 1U;
    sensor->init_status = BMP280_Init(&sensor->device, &bmp280_bus);
    result = map_status(sensor->init_status);
    if (result != SENSOR_RESULT_OK) release_bus(sensor);
    return result;
}

static void sensor_disconnect(void *context)
{
    release_bus((Bmp280Sensor *)context);
}

static SensorResult sensor_start(void *context)
{
    Bmp280Sensor *sensor = (Bmp280Sensor *)context;
    sensor->reading = BMP280_Read(&sensor->device);
    return map_status(sensor->reading.status);
}

static SensorResult sensor_poll(void *context)
{
    (void)context;
    return SENSOR_RESULT_HARDWARE_ERROR;
}

const SensorOps BMP280_SENSOR_OPS = {
    sensor_connect,
    sensor_disconnect,
    sensor_start,
    sensor_poll
};
