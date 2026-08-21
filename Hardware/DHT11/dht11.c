/**
 * @file    dht11.c
 * @brief   Platform-independent DHT11 protocol driver.
 */
#include "dht11.h"

#define DHT11_BIT_ONE_THRESHOLD_US 45U

DHT11_Status DHT11_Init(DHT11_Device *device, const DHT11_Port *port)
{
    if (device == 0 || port == 0 || port->init == 0 || port->deinit == 0 ||
        port->set_output == 0 || port->set_input == 0 || port->write == 0 ||
        port->delay_us == 0 || port->delay_ms == 0 ||
        port->critical_enter == 0 || port->critical_exit == 0 ||
        port->wait_while == 0) {
        return DHT11_ERR_INVALID_ARGUMENT;
    }

    device->port = *port;
    device->initialized = 0U;
    if (!device->port.init(device->port.context)) return DHT11_ERR_PORT;
    device->initialized = 1U;
    return DHT11_OK;
}

void DHT11_DeInit(DHT11_Device *device)
{
    if (device == 0) return;
    if (device->initialized != 0U) {
        device->port.deinit(device->port.context);
    }
    device->initialized = 0U;
}

DHT11_Reading DHT11_Read(DHT11_Device *device)
{
    DHT11_Reading reading = {0U, 0U, DHT11_OK};
    uint8_t data[5] = {0U, 0U, 0U, 0U, 0U};
    uint32_t pulse_us = 0U;
    uint32_t critical_state;
    uint8_t bit;

    if (device == 0 || device->initialized == 0U) {
        reading.status = DHT11_ERR_NOT_INITIALIZED;
        return reading;
    }

    device->port.set_output(device->port.context);
    device->port.write(device->port.context, false);
    device->port.delay_ms(device->port.context, 20U);
    device->port.write(device->port.context, true);
    device->port.set_input(device->port.context);
    device->port.delay_us(device->port.context, 30U);

    critical_state = device->port.critical_enter(device->port.context);

    if (!device->port.wait_while(device->port.context, true, 120U, 0)) {
        reading.status = DHT11_ERR_RESPONSE_HIGH;
        goto done;
    }
    if (!device->port.wait_while(device->port.context, false, 140U, 0)) {
        reading.status = DHT11_ERR_RESPONSE_LOW;
        goto done;
    }
    if (!device->port.wait_while(device->port.context, true, 140U, 0)) {
        reading.status = DHT11_ERR_RESPONSE_RELEASE;
        goto done;
    }

    for (bit = 0U; bit < 40U; bit++) {
        if (!device->port.wait_while(device->port.context, false, 90U, 0)) {
            reading.status = DHT11_ERR_BIT_LOW;
            goto done;
        }
        if (!device->port.wait_while(device->port.context, true, 120U,
                                     &pulse_us)) {
            reading.status = DHT11_ERR_BIT_HIGH;
            goto done;
        }
        data[bit / 8U] <<= 1;
        if (pulse_us > DHT11_BIT_ONE_THRESHOLD_US) data[bit / 8U] |= 1U;
    }

done:
    device->port.critical_exit(device->port.context, critical_state);

    if (reading.status == DHT11_OK) {
        uint8_t checksum = (uint8_t)(data[0] + data[1] + data[2] + data[3]);
        if (checksum != data[4]) {
            reading.status = DHT11_ERR_CHECKSUM;
        } else {
            reading.humidity = data[0];
            reading.temperature = data[2];
        }
    }
    return reading;
}

const char *DHT11_StatusText(DHT11_Status status)
{
    switch (status) {
        case DHT11_OK:                   return "ok";
        case DHT11_ERR_INVALID_ARGUMENT: return "invalid_argument";
        case DHT11_ERR_PORT:             return "port_error";
        case DHT11_ERR_NOT_INITIALIZED:  return "not_initialized";
        case DHT11_ERR_RESPONSE_HIGH:    return "response_high_timeout";
        case DHT11_ERR_RESPONSE_LOW:     return "response_low_timeout";
        case DHT11_ERR_RESPONSE_RELEASE: return "response_release_timeout";
        case DHT11_ERR_BIT_LOW:          return "bit_low_timeout";
        case DHT11_ERR_BIT_HIGH:         return "bit_high_timeout";
        case DHT11_ERR_CHECKSUM:         return "checksum_error";
        default:                         return "unknown";
    }
}
