/**
 * @file    timer.c — TIM4 编码器模式 (寄存器级)
 *          PB6=TIM4_CH1, PB7=TIM4_CH2, 4倍频计数
 */
#include "timer.h"

void TIM4_Encoder_Init(void)
{
    /* 使能时钟 */
    RCC->APB1ENR |= RCC_APB1ENR_TIM4EN;
    __HAL_RCC_GPIOB_CLK_ENABLE();

    /* PB6, PB7: 浮空输入 + 上拉 */
    GPIOB->CRL &= ~(0xFFu << 24);
    GPIOB->CRL |=  (0x88u << 24);           /* CNF=10, MODE=00 */
    GPIOB->ODR |=  (GPIO_ODR_ODR6 | GPIO_ODR_ODR7);  /* 上拉 */

    /* TIM4 编码器模式: SMS=011 (TI1+TI2 双沿, 4倍频) */
    TIM4->SMCR  = TIM_SMCR_SMS_0 | TIM_SMCR_SMS_1;
    TIM4->CCMR1 = TIM_CCMR1_CC1S_0 | TIM_CCMR1_CC2S_0;  /* CC1→TI1, CC2→TI2 */
    TIM4->CCER  = 0;
    TIM4->ARR   = 0xFFFF;
    TIM4->CNT   = 0;
    TIM4->CR1   = TIM_CR1_CEN;              /* 启动计数器 */
}

uint16_t TIM4_GetCount(void)
{
    return TIM4->CNT;
}

void TIM4_ResetCount(void)
{
    TIM4->CNT = 0;
}
