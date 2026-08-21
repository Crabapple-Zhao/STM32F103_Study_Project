# Sensor Integration Framework

The sensor implementation is split into three layers. Keep these boundaries when
adding a new sensor.

## Layers

1. `Hardware/<Sensor>`
   - Implements the device protocol and sensor-specific result codes.
   - The protocol core receives GPIO, delay, timer, or bus operations through a
     port structure when practical.
   - MCU-specific pin and peripheral ownership belongs in a separate
     `*_stm32f103.c` port file.
   - It must not depend on Astra UI or application page state.

2. `Application/Sensors`
   - Adapts a native sensor driver to `SensorOps`.
   - Owns the native device instance and latest reading.
   - Maps native errors to `SensorResult` without losing the native status used
     by diagnostics.
   - Acquires hardware resources in `connect` and releases them in `disconnect`.

3. `Astra/astra/pages`
   - Registers sensor menu entries and renders readings or error text.
   - Calls `SensorRuntime_Enter` when a level-3 page opens and
     `SensorRuntime_Exit` when it closes.
   - Calls `SensorRuntime_Update` while the page is active.
   - Does not access GPIO, timers, I2C, or a native sensor driver directly.

## Runtime States

| Sensor result | Runtime state | Default UI meaning |
|---|---|---|
| `SENSOR_RESULT_OK` | `SENSOR_STATE_READY` | Valid reading |
| `SENSOR_RESULT_BUSY` | `SENSOR_STATE_MEASURING` | Measurement in progress |
| `SENSOR_RESULT_NO_SENSOR` | `SENSOR_STATE_NO_SENSOR` | Sensor not detected |
| `SENSOR_RESULT_NO_DATA` | `SENSOR_STATE_NO_DATA` | Sensor detected, no sample |
| `SENSOR_RESULT_DATA_ERROR` | `SENSOR_STATE_DATA_ERROR` | Invalid sample or checksum |
| `SENSOR_RESULT_HARDWARE_ERROR` | `SENSOR_STATE_HARDWARE_ERROR` | Peripheral/resource failure |

`NO_SENSOR` and `HARDWARE_ERROR` release the backend and use the reconnect
interval. `NO_DATA` and `DATA_ERROR` keep the backend connected and retry at the
normal sample interval.

## Adding a Sensor

1. Add a portable driver under `Hardware/<Sensor>`.
2. Add an STM32F103 port file if the driver needs MCU-specific GPIO or timers.
3. Add an adapter under `Application/Sensors` and implement all four `SensorOps`:
   `connect`, `disconnect`, `start`, and `poll`.
4. Ensure every successful resource acquisition has a matching rollback path
   when `connect` fails and a matching release in `disconnect`.
5. Add the page callbacks and menu registration in `sensor_pages.cpp`.
6. Add all source files and include paths to `MDK-ARM/Project.uvprojx`.
7. Build, flash, and verify startup plus enter/read/exit/reconnect behavior from
   serial logs on real hardware.

## Shared Resources

- I2C2 is reference-counted through `I2C2_BusAcquire` and `I2C2_BusRelease`.
- CS100A owns TIM2 only while its page is active and refuses to overwrite an
  already enabled TIM2 configuration.
- DHT11 owns PA12 only while its page is active.
- The UI currently uses one static `SensorRuntime` because only one content page
  can be active at a time. Do not use it concurrently from multiple pages.
