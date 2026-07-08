/**
 * @file    key.c - key driver
 *          KEY1=PA0, KEY2=PC13, active high
 */
#include "key.h"

void KEY_Init(void)
{
    GPIO_InitTypeDef g = {0};

    /* KEY1 - PA0 */
    __HAL_RCC_GPIOA_CLK_ENABLE();
    g.Pin  = KEY1_PIN;
    g.Mode = GPIO_MODE_INPUT;
    g.Pull = GPIO_PULLDOWN;
    HAL_GPIO_Init(KEY1_PORT, &g);

    /* KEY2 - PC13 */
    __HAL_RCC_GPIOC_CLK_ENABLE();
    g.Pin  = KEY2_PIN;
    g.Mode = GPIO_MODE_INPUT;
    g.Pull = GPIO_PULLDOWN;
    HAL_GPIO_Init(KEY2_PORT, &g);
}

uint8_t KEY_Read(uint8_t key_num)
{
    switch (key_num)
    {
    case KEY_NUM_1:
        return (HAL_GPIO_ReadPin(KEY1_PORT, KEY1_PIN) == KEY_ACTIVE_LEVEL);
    case KEY_NUM_2:
        return (HAL_GPIO_ReadPin(KEY2_PORT, KEY2_PIN) == KEY_ACTIVE_LEVEL);
    default:
        return 0;
    }
}