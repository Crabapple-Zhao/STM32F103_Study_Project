/**
 * @file    i2c.c
 * @brief   I2C2 polling driver on PB10=SCL and PB11=SDA at 100 kHz.
 */
#include "i2c.h"

static I2C_HandleTypeDef i2c2_handle;
static uint8_t i2c2_initialized = 0U;
static uint8_t i2c2_users = 0U;

static void i2c2_hardware_deinit(void)
{
    GPIO_InitTypeDef gpio = {0};

    if (i2c2_initialized != 0U) HAL_I2C_DeInit(&i2c2_handle);

    gpio.Pin = GPIO_PIN_10 | GPIO_PIN_11;
    gpio.Mode = GPIO_MODE_INPUT;
    gpio.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOB, &gpio);

    __HAL_RCC_I2C2_CLK_DISABLE();
    i2c2_initialized = 0U;
}

bool I2C2_BusAcquire(void)
{
    GPIO_InitTypeDef gpio = {0};

    if (i2c2_initialized != 0U) {
        if (i2c2_users == 0xFFU) return false;
        i2c2_users++;
        return true;
    }

    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_I2C2_CLK_ENABLE();

    gpio.Pin = GPIO_PIN_10 | GPIO_PIN_11;
    gpio.Mode = GPIO_MODE_AF_OD;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOB, &gpio);

    i2c2_handle.Instance = I2C2;
    i2c2_handle.Init.ClockSpeed = 100000U;
    i2c2_handle.Init.DutyCycle = I2C_DUTYCYCLE_2;
    i2c2_handle.Init.OwnAddress1 = 0U;
    i2c2_handle.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    i2c2_handle.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    i2c2_handle.Init.OwnAddress2 = 0U;
    i2c2_handle.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    i2c2_handle.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;

    if (HAL_I2C_Init(&i2c2_handle) != HAL_OK) {
        i2c2_hardware_deinit();
        return false;
    }

    i2c2_initialized = 1U;
    i2c2_users = 1U;
    return true;
}

void I2C2_BusRelease(void)
{
    if (i2c2_users == 0U) return;
    i2c2_users--;
    if (i2c2_users != 0U) return;
    i2c2_hardware_deinit();
}

uint8_t I2C2_BusGetUserCount(void)
{
    return i2c2_users;
}

bool I2C2_BusInit(void)
{
    return I2C2_BusAcquire();
}

void I2C2_BusDeInit(void)
{
    I2C2_BusRelease();
}

bool I2C2_BusIsReady(uint8_t address7, uint32_t timeout_ms)
{
    if (i2c2_initialized == 0U) return false;
    return HAL_I2C_IsDeviceReady(&i2c2_handle, (uint16_t)(address7 << 1),
                                 2U, timeout_ms) == HAL_OK;
}

bool I2C2_BusMemRead(uint8_t address7, uint8_t reg, uint8_t *data,
                     uint16_t size, uint32_t timeout_ms)
{
    if (i2c2_initialized == 0U || data == 0 || size == 0U) return false;
    return HAL_I2C_Mem_Read(&i2c2_handle, (uint16_t)(address7 << 1), reg,
                            I2C_MEMADD_SIZE_8BIT, data, size, timeout_ms) == HAL_OK;
}

bool I2C2_BusMemWrite(uint8_t address7, uint8_t reg, const uint8_t *data,
                      uint16_t size, uint32_t timeout_ms)
{
    if (i2c2_initialized == 0U || data == 0 || size == 0U) return false;
    return HAL_I2C_Mem_Write(&i2c2_handle, (uint16_t)(address7 << 1), reg,
                             I2C_MEMADD_SIZE_8BIT, (uint8_t *)data,
                             size, timeout_ms) == HAL_OK;
}
