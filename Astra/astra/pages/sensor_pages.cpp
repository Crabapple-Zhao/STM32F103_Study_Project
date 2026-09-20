/**
 * @file    sensor_pages.cpp
 * @brief   Sensor page registry, rendering, logging and lifecycle binding.
 */
#include "sensor_pages.h"

#include "app_log.h"
#include "bmp280_sensor.h"
#include "cs100a_sensor.h"
#include "dht11_sensor.h"
#include "hal.h"
#include "i2c.h"
#include "sensor_runtime.h"
#include "ui/element/page/item.h"

#include <cstddef>

static Dht11Sensor dht11Sensor;
static Bmp280Sensor bmp280Sensor;
static Cs100aSensor cs100aSensor;

static SensorRuntime sensorRuntime;
static uint32_t loggedSequence = 0U;
static uint16_t loggedConnections = 0U;

static size_t asciiLength(const char *text)
{
    size_t len = 0U;
    while (text != 0 && text[len] != 0) len++;
    return len;
}

static void drawCenteredText(const char *text, float baselineY)
{
    HAL::drawEnglish(((float)APP_DISPLAY_WIDTH - (float)(asciiLength(text) * 8U)) / 2.0f,
                     baselineY, text);
}

static void appendText(char *buf, int *pos, const char *text)
{
    int i;
    for (i = 0; text[i] != 0; i++) buf[(*pos)++] = text[i];
}

static void appendUInt(char *buf, int *pos, uint32_t value)
{
    char tmp[10];
    int len = 0;

    if (value == 0U) {
        buf[(*pos)++] = '0';
        return;
    }
    while (value > 0U && len < 10) {
        tmp[len++] = (char)('0' + value % 10U);
        value /= 10U;
    }
    while (len > 0) buf[(*pos)++] = tmp[--len];
}

static void appendFixed2(char *buf, int *pos, int32_t value)
{
    uint32_t magnitude;
    if (value < 0) {
        buf[(*pos)++] = '-';
        magnitude = (uint32_t)(-(int64_t)value);
    } else {
        magnitude = (uint32_t)value;
    }

    appendUInt(buf, pos, magnitude / 100U);
    buf[(*pos)++] = '.';
    buf[(*pos)++] = (char)('0' + (magnitude / 10U) % 10U);
    buf[(*pos)++] = (char)('0' + magnitude % 10U);
}

static void appendHexByte(char *buf, int *pos, uint8_t value)
{
    static const char hex[] = "0123456789ABCDEF";
    buf[(*pos)++] = '0';
    buf[(*pos)++] = 'x';
    buf[(*pos)++] = hex[value >> 4];
    buf[(*pos)++] = hex[value & 0x0FU];
}

static void drawUInt(uint8_t x, uint8_t y, uint32_t value)
{
    char buf[11];
    int pos = 0;
    appendUInt(buf, &pos, value);
    buf[pos] = 0;
    HAL::drawEnglish(x, y, buf);
}

static void drawMeasurementCentered(const char *prefix, int32_t value,
                                    const char *suffix, uint8_t baselineY)
{
    char buf[32];
    int pos = 0;
    appendText(buf, &pos, prefix);
    appendFixed2(buf, &pos, value);
    appendText(buf, &pos, suffix);
    buf[pos] = 0;
    drawCenteredText(buf, baselineY);
}

static const char *displayStatus(SensorState state, const char *noDataText)
{
    switch (state) {
        case SENSOR_STATE_READY:          return "OK";
        case SENSOR_STATE_NO_SENSOR:      return "No Sensor";
        case SENSOR_STATE_NO_DATA:        return noDataText;
        case SENSOR_STATE_DATA_ERROR:     return "Data error";
        case SENSOR_STATE_HARDWARE_ERROR: return "Sensor error";
        case SENSOR_STATE_CONNECTING:
        case SENSOR_STATE_MEASURING:      return "Measuring";
        case SENSOR_STATE_INACTIVE:
        default:                          return "Sensor error";
    }
}

static void logDht11Connection(const char *event)
{
    char buf[144];
    int pos = 0;
    appendText(buf, &pos, "[INFO] [dht11] ");
    appendText(buf, &pos, event);
    appendText(buf, &pos, " connect=");
    appendText(buf, &pos, DHT11_StatusText(dht11Sensor.init_status));
    appendText(buf, &pos, " state=");
    appendText(buf, &pos, SensorStateText(sensorRuntime.state));
    appendText(buf, &pos, " pin=PA12 attempt=");
    appendUInt(buf, &pos, sensorRuntime.connection_attempts);
    appendText(buf, &pos, "\r\n");
    buf[pos] = 0;
    uart_puts(buf);
}

static void logDht11Reading(void)
{
    char buf[144];
    int pos = 0;
    appendText(buf, &pos, "[INFO] [dht11] status=");
    appendText(buf, &pos, displayStatus(sensorRuntime.state, "No data"));
    appendText(buf, &pos, " raw=");
    appendText(buf, &pos, DHT11_StatusText(dht11Sensor.reading.status));
    if (sensorRuntime.state == SENSOR_STATE_READY) {
        appendText(buf, &pos, " temp=");
        appendUInt(buf, &pos, dht11Sensor.reading.temperature);
        appendText(buf, &pos, " humi=");
        appendUInt(buf, &pos, dht11Sensor.reading.humidity);
    }
    appendText(buf, &pos, "\r\n");
    buf[pos] = 0;
    uart_puts(buf);
}

static void logBmp280Connection(const char *event)
{
    char buf[176];
    int pos = 0;
    appendText(buf, &pos, "[INFO] [bmp280] ");
    appendText(buf, &pos, event);
    appendText(buf, &pos, " connect=");
    appendText(buf, &pos, BMP280_StatusText(bmp280Sensor.init_status));
    appendText(buf, &pos, " state=");
    appendText(buf, &pos, SensorStateText(sensorRuntime.state));
    appendText(buf, &pos, " bus=I2C2 scl=PB10 sda=PB11 users=");
    appendUInt(buf, &pos, I2C2_BusGetUserCount());
    if (bmp280Sensor.init_status == BMP280_OK) {
        appendText(buf, &pos, " address=");
        appendHexByte(buf, &pos, BMP280_GetAddress(&bmp280Sensor.device));
        appendText(buf, &pos, " id=");
        appendHexByte(buf, &pos, BMP280_GetChipId(&bmp280Sensor.device));
    }
    appendText(buf, &pos, " attempt=");
    appendUInt(buf, &pos, sensorRuntime.connection_attempts);
    appendText(buf, &pos, "\r\n");
    buf[pos] = 0;
    uart_puts(buf);
}

static void logBmp280Reading(void)
{
    char buf[160];
    int pos = 0;
    appendText(buf, &pos, "[INFO] [bmp280] status=");
    appendText(buf, &pos, SensorStateText(sensorRuntime.state));
    appendText(buf, &pos, " raw=");
    appendText(buf, &pos, BMP280_StatusText(bmp280Sensor.reading.status));
    if (sensorRuntime.state == SENSOR_STATE_READY) {
        appendText(buf, &pos, " temp_c=");
        appendFixed2(buf, &pos, bmp280Sensor.reading.temperature_centi_c);
        appendText(buf, &pos, " pressure_pa=");
        appendUInt(buf, &pos, bmp280Sensor.reading.pressure_pa);
    }
    appendText(buf, &pos, "\r\n");
    buf[pos] = 0;
    uart_puts(buf);
}

static void logCs100aConnection(const char *event)
{
    char buf[176];
    int pos = 0;
    appendText(buf, &pos, "[INFO] [cs100a] ");
    appendText(buf, &pos, event);
    appendText(buf, &pos, " connect=");
    appendText(buf, &pos, CS100A_StatusText(cs100aSensor.init_status));
    appendText(buf, &pos, " state=");
    appendText(buf, &pos, SensorStateText(sensorRuntime.state));
    appendText(buf, &pos, " trig=PA15 echo=PB3 timer=TIM2_CH2 attempt=");
    appendUInt(buf, &pos, sensorRuntime.connection_attempts);
    appendText(buf, &pos, "\r\n");
    buf[pos] = 0;
    uart_puts(buf);
}

static void logCs100aReading(void)
{
    char buf[144];
    int pos = 0;
    appendText(buf, &pos, "[INFO] [cs100a] status=");
    appendText(buf, &pos, SensorStateText(sensorRuntime.state));
    appendText(buf, &pos, " raw=");
    appendText(buf, &pos, CS100A_StatusText(cs100aSensor.reading.status));
    if (cs100aSensor.reading.pulse_us != 0U) {
        appendText(buf, &pos, " pulse_us=");
        appendUInt(buf, &pos, cs100aSensor.reading.pulse_us);
    }
    if (sensorRuntime.state == SENSOR_STATE_READY) {
        appendText(buf, &pos, " distance_mm=");
        appendUInt(buf, &pos, cs100aSensor.reading.distance_mm);
    }
    appendText(buf, &pos, "\r\n");
    buf[pos] = 0;
    uart_puts(buf);
}

static void logRuntimeEvents(SensorRuntime *runtime,
                             uint32_t *loggedSequence,
                             uint16_t *loggedConnections,
                             void (*logConnection)(const char *),
                             void (*logReading)(void))
{
    if (runtime->connection_attempts != *loggedConnections) {
        logConnection(runtime->connection_attempts == 1U ? "page_enter" :
                                                         "reconnect");
        *loggedConnections = runtime->connection_attempts;
    }
    if (runtime->result_sequence != *loggedSequence) {
        logReading();
        *loggedSequence = runtime->result_sequence;
    }
}

static void dht11PageEnter(void)
{
    SensorRuntime_Init(&sensorRuntime, &DHT11_SENSOR_OPS, &dht11Sensor,
                       2000U, 2000U);
    loggedSequence = 0U;
    loggedConnections = 0U;
    SensorRuntime_Enter(&sensorRuntime, HAL_GetTick());
    logRuntimeEvents(&sensorRuntime, &loggedSequence,
                     &loggedConnections, logDht11Connection,
                     logDht11Reading);
}

static void dht11PageExit(void)
{
    SensorRuntime_Exit(&sensorRuntime);
    APP_LOG_INFO("dht11", "page_exit released=1 pin=PA12");
}

static void drawDht11Page(void)
{
    SensorRuntime_Update(&sensorRuntime, HAL_GetTick());
    logRuntimeEvents(&sensorRuntime, &loggedSequence,
                     &loggedConnections, logDht11Connection,
                     logDht11Reading);

    drawCenteredText("Temp/Humi", 34);
    HAL::drawHLine((APP_DISPLAY_WIDTH - 108) / 2, 42, 108);
    if (sensorRuntime.state != SENSOR_STATE_READY) {
        drawCenteredText(displayStatus(sensorRuntime.state, "No data"), 78);
        return;
    }

    const int offsetX = (APP_DISPLAY_WIDTH - 128) / 2;
    HAL::drawEnglish(offsetX + 18, 72, "Temp:");
    drawUInt(offsetX + 66, 72, dht11Sensor.reading.temperature);
    HAL::drawEnglish(offsetX + 90, 72, "C");
    HAL::drawEnglish(offsetX + 18, 98, "Humi:");
    drawUInt(offsetX + 66, 98, dht11Sensor.reading.humidity);
    HAL::drawEnglish(offsetX + 90, 98, "%");
}

static void bmp280PageEnter(void)
{
    SensorRuntime_Init(&sensorRuntime, &BMP280_SENSOR_OPS, &bmp280Sensor,
                       2000U, 2000U);
    loggedSequence = 0U;
    loggedConnections = 0U;
    SensorRuntime_Enter(&sensorRuntime, HAL_GetTick());
    logRuntimeEvents(&sensorRuntime, &loggedSequence,
                     &loggedConnections, logBmp280Connection,
                     logBmp280Reading);
}

static void bmp280PageExit(void)
{
    SensorRuntime_Exit(&sensorRuntime);
    APP_LOG_INFO("bmp280", "page_exit released=1 bus=I2C2");
}

static void drawBmp280Page(void)
{
    SensorRuntime_Update(&sensorRuntime, HAL_GetTick());
    logRuntimeEvents(&sensorRuntime, &loggedSequence,
                     &loggedConnections, logBmp280Connection,
                     logBmp280Reading);

    drawCenteredText("Barometer", 34);
    HAL::drawHLine((APP_DISPLAY_WIDTH - 108) / 2, 42, 108);
    if (sensorRuntime.state != SENSOR_STATE_READY) {
        drawCenteredText(displayStatus(sensorRuntime.state, "No data"), 78);
        return;
    }

    drawCenteredText("Pressure", 60);
    drawMeasurementCentered("", (int32_t)bmp280Sensor.reading.pressure_pa,
                            " hPa", 80);
    drawMeasurementCentered("Temp ",
                            bmp280Sensor.reading.temperature_centi_c,
                            " C", 102);
}

static void cs100aPageEnter(void)
{
    SensorRuntime_Init(&sensorRuntime, &CS100A_SENSOR_OPS, &cs100aSensor,
                       500U, 2000U);
    loggedSequence = 0U;
    loggedConnections = 0U;
    SensorRuntime_Enter(&sensorRuntime, HAL_GetTick());
    logRuntimeEvents(&sensorRuntime, &loggedSequence,
                     &loggedConnections, logCs100aConnection,
                     logCs100aReading);
}

static void cs100aPageExit(void)
{
    SensorRuntime_Exit(&sensorRuntime);
    APP_LOG_INFO("cs100a", "page_exit released=1 timer=TIM2");
}

static void drawCs100aPage(void)
{
    SensorRuntime_Update(&sensorRuntime, HAL_GetTick());
    logRuntimeEvents(&sensorRuntime, &loggedSequence,
                     &loggedConnections, logCs100aConnection,
                     logCs100aReading);

    drawCenteredText("Distance", 34);
    HAL::drawHLine((APP_DISPLAY_WIDTH - 108) / 2, 42, 108);
    if (sensorRuntime.state != SENSOR_STATE_READY) {
        drawCenteredText(displayStatus(sensorRuntime.state, "No Echo"), 78);
        return;
    }

    drawMeasurementCentered("",
                            (int32_t)(cs100aSensor.reading.distance_mm * 10U),
                            " cm", 78);

    char pulseText[24];
    int pos = 0;
    appendText(pulseText, &pos, "Echo ");
    appendUInt(pulseText, &pos, cs100aSensor.reading.pulse_us);
    appendText(pulseText, &pos, " us");
    pulseText[pos] = 0;
    drawCenteredText(pulseText, 102);
}

void SensorPages_AddToMenu(astra::Menu *menu)
{
    if (menu == 0) return;
    menu->addItem(new astra::Menu("-Temp/Humi", drawDht11Page,
                                  dht11PageEnter, dht11PageExit));
    menu->addItem(new astra::Menu("-Barometer", drawBmp280Page,
                                  bmp280PageEnter, bmp280PageExit));
    menu->addItem(new astra::Menu("-Distance", drawCs100aPage,
                                  cs100aPageEnter, cs100aPageExit));
}
