/**
 * @file dht11.c
 * @brief DHT11 single-wire temperature/humidity sensor on PA12.
 */
#include "dht11.h"

#define DHT11_GPIO_PORT GPIOA
#define DHT11_GPIO_PIN  GPIO_PIN_12
#define DHT11_BIT_ONE_THRESHOLD_US 45U

static DHT11_Reading dht11_last_reading = {0, 0, DHT11_ERR_RESPONSE_HIGH};
static uint8_t dht11_has_reading = 0;
static uint8_t dht11_initialized = 0;

static void dht11_delay_us(uint32_t us)
{
    uint32_t start = DWT->CYCCNT;
    uint32_t cycles = (SystemCoreClock / 1000000U) * us;
    while ((uint32_t)(DWT->CYCCNT - start) < cycles) {
    }
}

static void dht11_set_output(void)
{
    GPIO_InitTypeDef gpio = {0};
    gpio.Pin = DHT11_GPIO_PIN;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(DHT11_GPIO_PORT, &gpio);
}

static void dht11_set_input(void)
{
    GPIO_InitTypeDef gpio = {0};
    gpio.Pin = DHT11_GPIO_PIN;
    gpio.Mode = GPIO_MODE_INPUT;
    gpio.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(DHT11_GPIO_PORT, &gpio);
}

static void dht11_release_pin(void)
{
    GPIO_InitTypeDef gpio = {0};
    gpio.Pin = DHT11_GPIO_PIN;
    gpio.Mode = GPIO_MODE_INPUT;
    gpio.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(DHT11_GPIO_PORT, &gpio);
}

static GPIO_PinState dht11_read_pin(void)
{
    return HAL_GPIO_ReadPin(DHT11_GPIO_PORT, DHT11_GPIO_PIN);
}

static uint8_t dht11_wait_while(GPIO_PinState level, uint32_t timeout_us, uint32_t *duration_us)
{
    uint32_t start = DWT->CYCCNT;
    uint32_t timeout_cycles = (SystemCoreClock / 1000000U) * timeout_us;
    while (dht11_read_pin() == level) {
        if ((uint32_t)(DWT->CYCCNT - start) > timeout_cycles) return 0;
    }
    if (duration_us != 0) {
        *duration_us = (uint32_t)(DWT->CYCCNT - start) / (SystemCoreClock / 1000000U);
    }
    return 1;
}

void DHT11_Init(void)
{
    __HAL_RCC_GPIOA_CLK_ENABLE();
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    dht11_set_input();
    dht11_last_reading.status = DHT11_ERR_RESPONSE_HIGH;
    dht11_last_reading.humidity = 0;
    dht11_last_reading.temperature = 0;
    dht11_has_reading = 0;
    dht11_initialized = 1;
}

void DHT11_DeInit(void)
{
    if (!dht11_initialized) return;
    dht11_release_pin();
    dht11_last_reading.status = DHT11_ERR_RESPONSE_HIGH;
    dht11_last_reading.humidity = 0;
    dht11_last_reading.temperature = 0;
    dht11_has_reading = 0;
    dht11_initialized = 0;
}

DHT11_Reading DHT11_Read(void)
{
    if (!dht11_initialized) DHT11_Init();

    DHT11_Reading reading = {0, 0, DHT11_OK};
    uint8_t data[5] = {0, 0, 0, 0, 0};
    uint32_t pulse_us = 0;

    dht11_set_output();
    HAL_GPIO_WritePin(DHT11_GPIO_PORT, DHT11_GPIO_PIN, GPIO_PIN_RESET);
    HAL_Delay(20);
    HAL_GPIO_WritePin(DHT11_GPIO_PORT, DHT11_GPIO_PIN, GPIO_PIN_SET);
    dht11_set_input();
    dht11_delay_us(30);

    uint32_t primask = __get_PRIMASK();
    __disable_irq();

    if (!dht11_wait_while(GPIO_PIN_SET, 120, 0)) {
        reading.status = DHT11_ERR_RESPONSE_HIGH;
        goto done;
    }
    if (!dht11_wait_while(GPIO_PIN_RESET, 140, 0)) {
        reading.status = DHT11_ERR_RESPONSE_LOW;
        goto done;
    }
    if (!dht11_wait_while(GPIO_PIN_SET, 140, 0)) {
        reading.status = DHT11_ERR_RESPONSE_RELEASE;
        goto done;
    }

    for (uint8_t bit = 0; bit < 40; bit++) {
        if (!dht11_wait_while(GPIO_PIN_RESET, 90, 0)) {
            reading.status = DHT11_ERR_BIT_LOW;
            goto done;
        }
        if (!dht11_wait_while(GPIO_PIN_SET, 120, &pulse_us)) {
            reading.status = DHT11_ERR_BIT_HIGH;
            goto done;
        }
        data[bit / 8] <<= 1;
        if (pulse_us > DHT11_BIT_ONE_THRESHOLD_US) data[bit / 8] |= 1U;
    }

done:
    if (primask == 0U) __enable_irq();

    if (reading.status == DHT11_OK) {
        uint8_t checksum = (uint8_t)(data[0] + data[1] + data[2] + data[3]);
        if (checksum != data[4]) {
            reading.status = DHT11_ERR_CHECKSUM;
        } else {
            reading.humidity = data[0];
            reading.temperature = data[2];
        }
    }

    dht11_last_reading = reading;
    dht11_has_reading = 1;

    return reading;
}

uint8_t DHT11_GetLastReading(DHT11_Reading *reading)
{
    if (reading != 0) *reading = dht11_last_reading;
    return dht11_has_reading;
}

const char *DHT11_StatusText(DHT11_Status status)
{
    switch (status) {
        case DHT11_OK: return "ok";
        case DHT11_ERR_RESPONSE_HIGH: return "response_high_timeout";
        case DHT11_ERR_RESPONSE_LOW: return "response_low_timeout";
        case DHT11_ERR_RESPONSE_RELEASE: return "response_release_timeout";
        case DHT11_ERR_BIT_LOW: return "bit_low_timeout";
        case DHT11_ERR_BIT_HIGH: return "bit_high_timeout";
        case DHT11_ERR_CHECKSUM: return "checksum_error";
        default: return "unknown";
    }
}
