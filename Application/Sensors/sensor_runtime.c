/**
 * @file    sensor_runtime.c
 * @brief   Shared lifecycle and sampling state machine for sensor pages.
 */
#include "sensor_runtime.h"

static SensorState result_to_state(SensorResult result)
{
    switch (result) {
        case SENSOR_RESULT_OK:             return SENSOR_STATE_READY;
        case SENSOR_RESULT_BUSY:           return SENSOR_STATE_MEASURING;
        case SENSOR_RESULT_NO_SENSOR:      return SENSOR_STATE_NO_SENSOR;
        case SENSOR_RESULT_NO_DATA:        return SENSOR_STATE_NO_DATA;
        case SENSOR_RESULT_DATA_ERROR:     return SENSOR_STATE_DATA_ERROR;
        case SENSOR_RESULT_HARDWARE_ERROR: return SENSOR_STATE_HARDWARE_ERROR;
        default:                           return SENSOR_STATE_HARDWARE_ERROR;
    }
}

static void disconnect_backend(SensorRuntime *runtime)
{
    if (runtime->connected != 0U) {
        runtime->ops->disconnect(runtime->context);
    }
    runtime->connected = 0U;
    runtime->measuring = 0U;
}

static void finish_result(SensorRuntime *runtime, SensorResult result,
                          uint32_t now_ms)
{
    runtime->state = result_to_state(result);
    runtime->measuring = 0U;
    runtime->has_result = 1U;
    runtime->last_action_ms = now_ms;
    runtime->result_sequence++;

    if (result == SENSOR_RESULT_NO_SENSOR ||
        result == SENSOR_RESULT_HARDWARE_ERROR) {
        disconnect_backend(runtime);
    }
}

static void start_measurement(SensorRuntime *runtime, uint32_t now_ms)
{
    SensorResult result = runtime->ops->start(runtime->context);
    if (result == SENSOR_RESULT_BUSY) {
        if (runtime->has_result == 0U) {
            runtime->state = SENSOR_STATE_MEASURING;
        }
        runtime->measuring = 1U;
        return;
    }
    finish_result(runtime, result, now_ms);
}

static void connect_backend(SensorRuntime *runtime, uint32_t now_ms)
{
    SensorResult result;

    runtime->state = SENSOR_STATE_CONNECTING;
    runtime->last_action_ms = now_ms;
    runtime->connection_attempts++;
    result = runtime->ops->connect(runtime->context);
    if (result == SENSOR_RESULT_OK) {
        runtime->connected = 1U;
        start_measurement(runtime, now_ms);
        return;
    }
    finish_result(runtime, result, now_ms);
}

bool SensorRuntime_Init(SensorRuntime *runtime, const SensorOps *ops,
                        void *context, uint32_t sample_interval_ms,
                        uint32_t reconnect_interval_ms)
{
    if (runtime == 0 || ops == 0 || ops->connect == 0 ||
        ops->disconnect == 0 || ops->start == 0 || ops->poll == 0 ||
        sample_interval_ms == 0U || reconnect_interval_ms == 0U) {
        return false;
    }

    runtime->ops = ops;
    runtime->context = context;
    runtime->sample_interval_ms = sample_interval_ms;
    runtime->reconnect_interval_ms = reconnect_interval_ms;
    runtime->last_action_ms = 0U;
    runtime->result_sequence = 0U;
    runtime->connection_attempts = 0U;
    runtime->state = SENSOR_STATE_INACTIVE;
    runtime->active = 0U;
    runtime->connected = 0U;
    runtime->measuring = 0U;
    runtime->has_result = 0U;
    return true;
}

void SensorRuntime_Enter(SensorRuntime *runtime, uint32_t now_ms)
{
    if (runtime == 0 || runtime->ops == 0) return;
    if (runtime->active != 0U) SensorRuntime_Exit(runtime);

    runtime->active = 1U;
    runtime->connected = 0U;
    runtime->measuring = 0U;
    runtime->has_result = 0U;
    runtime->result_sequence = 0U;
    runtime->connection_attempts = 0U;
    connect_backend(runtime, now_ms);
}

void SensorRuntime_Exit(SensorRuntime *runtime)
{
    if (runtime == 0 || runtime->ops == 0) return;
    disconnect_backend(runtime);
    runtime->state = SENSOR_STATE_INACTIVE;
    runtime->active = 0U;
    runtime->has_result = 0U;
    runtime->last_action_ms = 0U;
}

void SensorRuntime_Update(SensorRuntime *runtime, uint32_t now_ms)
{
    SensorResult result;

    if (runtime == 0 || runtime->active == 0U) return;

    if (runtime->connected == 0U) {
        if ((uint32_t)(now_ms - runtime->last_action_ms) >=
            runtime->reconnect_interval_ms) {
            connect_backend(runtime, now_ms);
        }
        return;
    }

    if (runtime->measuring != 0U) {
        result = runtime->ops->poll(runtime->context);
        if (result != SENSOR_RESULT_BUSY) finish_result(runtime, result, now_ms);
        return;
    }

    if ((uint32_t)(now_ms - runtime->last_action_ms) >=
        runtime->sample_interval_ms) {
        runtime->last_action_ms = now_ms;
        start_measurement(runtime, now_ms);
    }
}

const char *SensorStateText(SensorState state)
{
    switch (state) {
        case SENSOR_STATE_INACTIVE:       return "inactive";
        case SENSOR_STATE_CONNECTING:     return "connecting";
        case SENSOR_STATE_MEASURING:      return "measuring";
        case SENSOR_STATE_READY:          return "ok";
        case SENSOR_STATE_NO_SENSOR:      return "no_sensor";
        case SENSOR_STATE_NO_DATA:        return "no_data";
        case SENSOR_STATE_DATA_ERROR:     return "data_error";
        case SENSOR_STATE_HARDWARE_ERROR: return "hardware_error";
        default:                          return "unknown";
    }
}
