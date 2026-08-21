/**
 * @file    bmp280.c
 * @brief   Portable BMP280 driver using caller-provided bus callbacks.
 */
#include "bmp280.h"

#define BMP280_ADDRESS_LOW       0x76U
#define BMP280_ADDRESS_HIGH      0x77U
#define BMP280_EXPECTED_CHIP_ID  0x58U

#define BMP280_REG_CALIB         0x88U
#define BMP280_REG_CHIP_ID       0xD0U
#define BMP280_REG_CTRL_MEAS     0xF4U
#define BMP280_REG_CONFIG        0xF5U
#define BMP280_REG_PRESS_MSB     0xF7U

#define BMP280_BUS_TIMEOUT_MS    50U

static uint16_t read_u16_le(const uint8_t *data)
{
    return (uint16_t)((uint16_t)data[0] | ((uint16_t)data[1] << 8));
}

static int16_t read_s16_le(const uint8_t *data)
{
    return (int16_t)read_u16_le(data);
}

static bool bmp280_read(BMP280_Device *device, uint8_t reg,
                        uint8_t *data, uint16_t size)
{
    return device->bus.read(device->bus.context, device->address, reg, data,
                            size, BMP280_BUS_TIMEOUT_MS);
}

static bool bmp280_write(BMP280_Device *device, uint8_t reg,
                         const uint8_t *data, uint16_t size)
{
    return device->bus.write(device->bus.context, device->address, reg, data,
                             size, BMP280_BUS_TIMEOUT_MS);
}

static bool bmp280_read_calibration(BMP280_Device *device)
{
    uint8_t data[24];
    BMP280_Calibration *calibration = &device->calibration;

    if (!bmp280_read(device, BMP280_REG_CALIB, data, sizeof(data))) return false;

    calibration->t1 = read_u16_le(&data[0]);
    calibration->t2 = read_s16_le(&data[2]);
    calibration->t3 = read_s16_le(&data[4]);
    calibration->p1 = read_u16_le(&data[6]);
    calibration->p2 = read_s16_le(&data[8]);
    calibration->p3 = read_s16_le(&data[10]);
    calibration->p4 = read_s16_le(&data[12]);
    calibration->p5 = read_s16_le(&data[14]);
    calibration->p6 = read_s16_le(&data[16]);
    calibration->p7 = read_s16_le(&data[18]);
    calibration->p8 = read_s16_le(&data[20]);
    calibration->p9 = read_s16_le(&data[22]);

    return calibration->t1 != 0U && calibration->t1 != 0xFFFFU &&
           calibration->p1 != 0U && calibration->p1 != 0xFFFFU;
}

BMP280_Status BMP280_Init(BMP280_Device *device, const BMP280_Bus *bus)
{
    uint8_t config = 0x10U;
    uint8_t ctrl_meas = 0x57U;

    if (device == 0 || bus == 0 || bus->is_ready == 0 || bus->read == 0 ||
        bus->write == 0 || bus->delay_ms == 0) {
        return BMP280_ERR_INVALID_ARGUMENT;
    }

    device->bus = *bus;
    device->address = 0U;
    device->chip_id = 0U;
    device->initialized = 0U;

    if (device->bus.is_ready(device->bus.context, BMP280_ADDRESS_LOW,
                             BMP280_BUS_TIMEOUT_MS)) {
        device->address = BMP280_ADDRESS_LOW;
    } else if (device->bus.is_ready(device->bus.context, BMP280_ADDRESS_HIGH,
                                    BMP280_BUS_TIMEOUT_MS)) {
        device->address = BMP280_ADDRESS_HIGH;
    } else {
        return BMP280_ERR_NO_SENSOR;
    }

    if (!bmp280_read(device, BMP280_REG_CHIP_ID, &device->chip_id, 1U)) {
        return BMP280_ERR_CHIP_ID;
    }
    if (device->chip_id != BMP280_EXPECTED_CHIP_ID) {
        return BMP280_ERR_CHIP_ID;
    }
    if (!bmp280_read_calibration(device)) return BMP280_ERR_CALIBRATION;

    if (!bmp280_write(device, BMP280_REG_CONFIG, &config, 1U)) {
        return BMP280_ERR_CONFIG;
    }
    if (!bmp280_write(device, BMP280_REG_CTRL_MEAS, &ctrl_meas, 1U)) {
        return BMP280_ERR_CONFIG;
    }

    device->initialized = 1U;
    device->bus.delay_ms(device->bus.context, 50U);
    return BMP280_OK;
}

void BMP280_DeInit(BMP280_Device *device)
{
    uint8_t sleep = 0U;

    if (device == 0) return;
    if (device->initialized != 0U) {
        bmp280_write(device, BMP280_REG_CTRL_MEAS, &sleep, 1U);
    }
    device->initialized = 0U;
}

BMP280_Reading BMP280_Read(BMP280_Device *device)
{
    BMP280_Reading reading = {0, 0U, BMP280_OK};
    uint8_t data[6];
    int32_t adc_pressure;
    int32_t adc_temperature;
    int32_t var1_t;
    int32_t var2_t;
    int32_t t_fine;
    int64_t var1_p;
    int64_t var2_p;
    int64_t pressure;
    BMP280_Calibration *calibration;

    if (device == 0 || device->initialized == 0U) {
        reading.status = BMP280_ERR_NOT_INITIALIZED;
        return reading;
    }
    calibration = &device->calibration;

    if (!bmp280_read(device, BMP280_REG_PRESS_MSB, data, sizeof(data))) {
        reading.status = BMP280_ERR_BUS;
        return reading;
    }

    adc_pressure = ((int32_t)data[0] << 12) |
                   ((int32_t)data[1] << 4) |
                   ((int32_t)data[2] >> 4);
    adc_temperature = ((int32_t)data[3] << 12) |
                      ((int32_t)data[4] << 4) |
                      ((int32_t)data[5] >> 4);

    if (adc_pressure == 0x80000L || adc_temperature == 0x80000L) {
        reading.status = BMP280_ERR_DATA;
        return reading;
    }

    var1_t = ((((adc_temperature >> 3) - ((int32_t)calibration->t1 << 1))) *
              (int32_t)calibration->t2) >> 11;
    var2_t = (((((adc_temperature >> 4) - (int32_t)calibration->t1) *
                ((adc_temperature >> 4) - (int32_t)calibration->t1)) >> 12) *
              (int32_t)calibration->t3) >> 14;
    t_fine = var1_t + var2_t;
    reading.temperature_centi_c = (t_fine * 5 + 128) >> 8;

    var1_p = (int64_t)t_fine - 128000LL;
    var2_p = var1_p * var1_p * (int64_t)calibration->p6;
    var2_p += (var1_p * (int64_t)calibration->p5) << 17;
    var2_p += ((int64_t)calibration->p4) << 35;
    var1_p = ((var1_p * var1_p * (int64_t)calibration->p3) >> 8) +
             ((var1_p * (int64_t)calibration->p2) << 12);
    var1_p = (((1LL << 47) + var1_p) * (int64_t)calibration->p1) >> 33;

    if (var1_p == 0) {
        reading.status = BMP280_ERR_COMPENSATION;
        return reading;
    }

    pressure = 1048576LL - (int64_t)adc_pressure;
    pressure = (((pressure << 31) - var2_p) * 3125LL) / var1_p;
    var1_p = ((int64_t)calibration->p9 * (pressure >> 13) *
              (pressure >> 13)) >> 25;
    var2_p = ((int64_t)calibration->p8 * pressure) >> 19;
    pressure = ((pressure + var1_p + var2_p) >> 8) +
               ((int64_t)calibration->p7 << 4);

    if (pressure <= 0 || pressure > ((int64_t)0xFFFFFFFFULL << 8)) {
        reading.status = BMP280_ERR_COMPENSATION;
        return reading;
    }

    reading.pressure_pa = (uint32_t)(pressure >> 8);
    return reading;
}

uint8_t BMP280_GetAddress(const BMP280_Device *device)
{
    return device == 0 ? 0U : device->address;
}

uint8_t BMP280_GetChipId(const BMP280_Device *device)
{
    return device == 0 ? 0U : device->chip_id;
}

const char *BMP280_StatusText(BMP280_Status status)
{
    switch (status) {
        case BMP280_OK:                   return "ok";
        case BMP280_ERR_INVALID_ARGUMENT: return "invalid_argument";
        case BMP280_ERR_BUS:              return "bus_error";
        case BMP280_ERR_NO_SENSOR:        return "no_sensor";
        case BMP280_ERR_CHIP_ID:          return "chip_id_error";
        case BMP280_ERR_CALIBRATION:      return "calibration_error";
        case BMP280_ERR_CONFIG:           return "config_error";
        case BMP280_ERR_NOT_INITIALIZED:  return "not_initialized";
        case BMP280_ERR_DATA:             return "data_error";
        case BMP280_ERR_COMPENSATION:     return "compensation_error";
        default:                          return "unknown";
    }
}
