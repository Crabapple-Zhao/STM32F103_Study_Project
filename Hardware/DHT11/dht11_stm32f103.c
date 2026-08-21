/**
 * @file    dht11_stm32f103.c
 * @brief   STM32F103 PA12 port for the DHT11 driver.
 */
#include "dht11_stm32f103.h"
#include "stm32f1xx_hal.h"

#define DHT11_GPIO_PORT GPIOA
#define DHT11_GPIO_PIN  GPIO_PIN_12

static uint32_t cycles_per_us = 0U;
static uint8_t port_initialized = 0U;

static bool port_init(void *context)
{
    GPIO_InitTypeDef gpio = {0};
    (void)context;

    __HAL_RCC_GPIOA_CLK_ENABLE();
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    cycles_per_us = SystemCoreClock / 1000000U;
    if (cycles_per_us == 0U) return false;

    gpio.Pin = DHT11_GPIO_PIN;
    gpio.Mode = GPIO_MODE_INPUT;
    gpio.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(DHT11_GPIO_PORT, &gpio);
    port_initialized = 1U;
    return true;
}

static void port_deinit(void *context)
{
    GPIO_InitTypeDef gpio = {0};
    (void)context;
    if (port_initialized == 0U) return;

    gpio.Pin = DHT11_GPIO_PIN;
    gpio.Mode = GPIO_MODE_INPUT;
    gpio.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(DHT11_GPIO_PORT, &gpio);
    port_initialized = 0U;
}

static void port_set_output(void *context)
{
    GPIO_InitTypeDef gpio = {0};
    (void)context;
    gpio.Pin = DHT11_GPIO_PIN;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(DHT11_GPIO_PORT, &gpio);
}

static void port_set_input(void *context)
{
    GPIO_InitTypeDef gpio = {0};
    (void)context;
    gpio.Pin = DHT11_GPIO_PIN;
    gpio.Mode = GPIO_MODE_INPUT;
    gpio.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(DHT11_GPIO_PORT, &gpio);
}

static void port_write(void *context, bool high)
{
    (void)context;
    HAL_GPIO_WritePin(DHT11_GPIO_PORT, DHT11_GPIO_PIN,
                      high ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

static void port_delay_us(void *context, uint32_t delay_us)
{
    uint32_t start = DWT->CYCCNT;
    (void)context;
    while ((uint32_t)(DWT->CYCCNT - start) < cycles_per_us * delay_us) {
    }
}

static void port_delay_ms(void *context, uint32_t delay_ms)
{
    (void)context;
    HAL_Delay(delay_ms);
}

static uint32_t port_critical_enter(void *context)
{
    uint32_t state;
    (void)context;
    state = __get_PRIMASK();
    __disable_irq();
    return state;
}

static void port_critical_exit(void *context, uint32_t state)
{
    (void)context;
    if (state == 0U) __enable_irq();
}

static bool port_wait_while(void *context, bool high, uint32_t timeout_us,
                            uint32_t *duration_us)
{
    GPIO_PinState level = high ? GPIO_PIN_SET : GPIO_PIN_RESET;
    uint32_t start = DWT->CYCCNT;
    uint32_t timeout_cycles = cycles_per_us * timeout_us;
    (void)context;

    while (HAL_GPIO_ReadPin(DHT11_GPIO_PORT, DHT11_GPIO_PIN) == level) {
        if ((uint32_t)(DWT->CYCCNT - start) > timeout_cycles) return false;
    }
    if (duration_us != 0) {
        *duration_us = (uint32_t)(DWT->CYCCNT - start) / cycles_per_us;
    }
    return true;
}

static const DHT11_Port dht11_port = {
    0,
    port_init,
    port_deinit,
    port_set_output,
    port_set_input,
    port_write,
    port_delay_us,
    port_delay_ms,
    port_critical_enter,
    port_critical_exit,
    port_wait_while
};

const DHT11_Port *DHT11_STM32F103_GetPort(void)
{
    return &dht11_port;
}
