/**
 * @file    cs100a.c
 * @brief   CS100A measurement state and distance conversion.
 */
#include "cs100a.h"

#define CS100A_NO_ECHO_PULSE_US  32900U

CS100A_Status CS100A_Init(CS100A_Device *device, const CS100A_Port *port)
{
    if (device == 0 || port == 0 || port->init == 0 || port->deinit == 0 ||
        port->start == 0 || port->poll == 0) {
        return CS100A_ERR_PORT;
    }

    device->port = *port;
    device->initialized = 0U;
    device->measuring = 0U;
    if (!device->port.init(device->port.context)) return CS100A_ERR_PORT;

    device->initialized = 1U;
    return CS100A_OK;
}

void CS100A_DeInit(CS100A_Device *device)
{
    if (device == 0) return;
    if (device->initialized != 0U) {
        device->port.deinit(device->port.context);
    }
    device->initialized = 0U;
    device->measuring = 0U;
}

CS100A_Status CS100A_Start(CS100A_Device *device)
{
    if (device == 0 || device->initialized == 0U) {
        return CS100A_ERR_NOT_INITIALIZED;
    }
    if (device->measuring != 0U) return CS100A_BUSY;
    if (!device->port.start(device->port.context)) return CS100A_ERR_PORT;

    device->measuring = 1U;
    return CS100A_BUSY;
}

CS100A_Reading CS100A_Poll(CS100A_Device *device)
{
    CS100A_Reading reading = {0U, 0U, CS100A_BUSY};
    CS100A_IO_Status io_status;

    if (device == 0 || device->initialized == 0U) {
        reading.status = CS100A_ERR_NOT_INITIALIZED;
        return reading;
    }
    if (device->measuring == 0U) return reading;

    io_status = device->port.poll(device->port.context, &reading.pulse_us);
    if (io_status == CS100A_IO_BUSY) return reading;

    device->measuring = 0U;
    if (io_status == CS100A_IO_TIMEOUT) {
        reading.status = CS100A_ERR_NO_ECHO;
        return reading;
    }
    if (io_status != CS100A_IO_READY) {
        reading.status = CS100A_ERR_PORT;
        return reading;
    }
    if (reading.pulse_us >= CS100A_NO_ECHO_PULSE_US) {
        reading.status = CS100A_ERR_NO_ECHO;
        return reading;
    }

    /* 340 m/s round trip: distance_mm = pulse_us * 0.17. */
    reading.distance_mm = (reading.pulse_us * 17U + 50U) / 100U;
    reading.status = CS100A_OK;
    return reading;
}

const char *CS100A_StatusText(CS100A_Status status)
{
    switch (status) {
        case CS100A_OK:                  return "ok";
        case CS100A_BUSY:                return "busy";
        case CS100A_ERR_NO_ECHO:         return "no_echo";
        case CS100A_ERR_PORT:            return "port_error";
        case CS100A_ERR_NOT_INITIALIZED: return "not_initialized";
        default:                         return "unknown";
    }
}
