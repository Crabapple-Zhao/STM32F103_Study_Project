/**
 * @file    encoder.c — 编码器旋钮驱动
 *          A=PB6(TIM4_CH1), B=PB7(TIM4_CH2), SW=PB5(上拉, 按下低电平)
 */
#include "encoder.h"
#include "timer.h"

void Encoder_Init(void)
{
    /* TIM4 编码器模式 */
    TIM4_Encoder_Init();

    /* SW 按键: PB5 上拉输入 */
    __HAL_RCC_GPIOB_CLK_ENABLE();
    GPIO_InitTypeDef g = {0};
    g.Pin  = ENC_SW_PIN;
    g.Mode = GPIO_MODE_INPUT;
    g.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(ENC_SW_PORT, &g);
}

int16_t Encoder_GetCount(void)
{
    return (int16_t)TIM4_GetCount();
}

void Encoder_ResetCount(void)
{
    TIM4_ResetCount();
}

uint8_t Encoder_SW_Read(void)
{
    /* 低电平 = 按下 */
    return (HAL_GPIO_ReadPin(ENC_SW_PORT, ENC_SW_PIN) == GPIO_PIN_RESET);
}
