/**
 * @file    sensor_runtime.h
 * @brief   Shared lifecycle and sampling state machine for sensor pages.
 */
#ifndef __SENSOR_RUNTIME_H
#define __SENSOR_RUNTIME_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    SENSOR_RESULT_OK = 0,
    SENSOR_RESULT_BUSY,
    SENSOR_RESULT_NO_SENSOR,
    SENSOR_RESULT_NO_DATA,
    SENSOR_RESULT_DATA_ERROR,
    SENSOR_RESULT_HARDWARE_ERROR
} SensorResult;

typedef enum {
    SENSOR_STATE_INACTIVE = 0,
    SENSOR_STATE_CONNECTING,
    SENSOR_STATE_MEASURING,
    SENSOR_STATE_READY,
    SENSOR_STATE_NO_SENSOR,
    SENSOR_STATE_NO_DATA,
    SENSOR_STATE_DATA_ERROR,
    SENSOR_STATE_HARDWARE_ERROR
} SensorState;

typedef struct {
    SensorResult (*connect)(void *context);
    void (*disconnect)(void *context);
    SensorResult (*start)(void *context);
    SensorResult (*poll)(void *context);
} SensorOps;

typedef struct {
    const SensorOps *ops;
    void *context;
    uint32_t sample_interval_ms;
    uint32_t reconnect_interval_ms;
    uint32_t last_action_ms;
    uint32_t result_sequence;
    uint16_t connection_attempts;
    SensorState state;
    uint8_t active;
    uint8_t connected;
    uint8_t measuring;
    uint8_t has_result;
} SensorRuntime;

bool SensorRuntime_Init(SensorRuntime *runtime, const SensorOps *ops,
                        void *context, uint32_t sample_interval_ms,
                        uint32_t reconnect_interval_ms);
void SensorRuntime_Enter(SensorRuntime *runtime, uint32_t now_ms);
void SensorRuntime_Exit(SensorRuntime *runtime);
void SensorRuntime_Update(SensorRuntime *runtime, uint32_t now_ms);
const char *SensorStateText(SensorState state);

#ifdef __cplusplus
}
#endif

#endif
