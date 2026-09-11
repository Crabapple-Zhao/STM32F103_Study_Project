/**
 * @file    pwm.c — TIM3_CH4 PWM 输出驱动 (寄存器级)
 *          PB1=TIM3_CH4, 预设频率档位, 占空比 0~100% (按档位精度)
 *
 * 关闭输出策略: CC4E 始终使能, 关闭时 CCR4=0 (PWM1 模式下恒低),
 * 引脚全程由定时器驱动, 关闭即真实 0V, 不依赖 GPIO 回退状态。
 */
#include "pwm.h"

/* PB1 在 GPIOB->CRL 的位偏移 (pin1 * 4) */
#define PWM_PB1_CRL_SHIFT    4U
/* 复用推挽输出 2MHz: CNF=10, MODE=10 (0b1010) */
#define PWM_PB1_AFPP_2MHZ    0xAu
/* 浮空输入: CNF=01, MODE=00 */
#define PWM_PB1_FLOAT_IN     0x4U

typedef struct {
    uint16_t psc;
    uint16_t arr;
    uint32_t freqHz;
} PWM_FreqConfig;

/* PSC 固定 71 (1MHz 计数), ARR 决定频率; 占空比精度 = 1/(ARR+1) */
static const PWM_FreqConfig pwmFreqTable[PWM_FREQ_NUM] = {
    { 71U, 9999U, 100UL },     /* 100Hz  */
    { 71U, 999U,  1000UL },    /* 1kHz   */
    { 71U, 99U,   10000UL },   /* 10kHz  */
    { 71U, 9U,    100000UL },  /* 100kHz */
};

static uint8_t pwmDutyPercent = 0U;
static uint8_t pwmEnabled = 0U;
static PWM_FreqPreset pwmFreqPreset = PWM_FREQ_1KHZ;

/* 将占空比写入比较寄存器: CCR4 = duty% * (ARR+1) / 100 (四舍五入) */
static void pwmApplyCompare(void)
{
    uint32_t period = (uint32_t)TIM3->ARR + 1U;
    TIM3->CCR4 = ((uint32_t)pwmDutyPercent * period + 50U) / 100U;
}

void PWM_TIM3_Init(void)
{
    /* 使能时钟 */
    RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;
    __HAL_RCC_GPIOB_CLK_ENABLE();

    /* PB1: 复用推挽输出 */
    GPIOB->CRL &= ~(0xFu << PWM_PB1_CRL_SHIFT);
    GPIOB->CRL |=  (PWM_PB1_AFPP_2MHZ << PWM_PB1_CRL_SHIFT);

    pwmDutyPercent = 0U;
    pwmEnabled = 0U;
    pwmFreqPreset = PWM_FREQ_1KHZ;

    const PWM_FreqConfig *cfg = &pwmFreqTable[pwmFreqPreset];
    TIM3->PSC = cfg->psc;
    TIM3->ARR = cfg->arr;

    /* CH4: PWM1 模式 (OC4M=110), 不预装载 (立即更新占空比) */
    TIM3->CCMR2 = TIM_CCMR2_OC4M_2 | TIM_CCMR2_OC4M_1;
    /* 使能 CH4 输出, 高电平有效 (CC4P=0) */
    TIM3->CCER = TIM_CCER_CC4E;

    TIM3->CCR4 = 0U;                /* 初始占空比 0% (恒低) */
    TIM3->EGR  = TIM_EGR_UG;       /* 产生更新事件, 立即装载 PSC/ARR */
    TIM3->CR1  = TIM_CR1_CEN;      /* 启动计数器 */
}

void PWM_TIM3_DeInit(void)
{
    TIM3->CR1 = 0U;                /* 停止计数器 */
    TIM3->CCER = 0U;               /* 关闭通道输出 */
    TIM3->CNT = 0U;
    TIM3->CCR4 = 0U;

    pwmDutyPercent = 0U;
    pwmEnabled = 0U;
    pwmFreqPreset = PWM_FREQ_1KHZ;

    RCC->APB1ENR &= ~RCC_APB1ENR_TIM3EN;

    /* PB1 释放为浮空输入 */
    GPIOB->CRL &= ~(0xFu << PWM_PB1_CRL_SHIFT);
    GPIOB->CRL |=  (PWM_PB1_FLOAT_IN << PWM_PB1_CRL_SHIFT);
}

void PWM_TIM3_SetDuty(uint8_t dutyPercent)
{
    if (dutyPercent > 100U) dutyPercent = 100U;
    pwmDutyPercent = dutyPercent;
    if (pwmEnabled != 0U) pwmApplyCompare();
}

void PWM_TIM3_SetEnable(uint8_t on)
{
    pwmEnabled = (on != 0U) ? 1U : 0U;
    if (pwmEnabled != 0U) pwmApplyCompare();  /* 恢复设定占空比 */
    else TIM3->CCR4 = 0U;                     /* 恒低 = 0V */
}

void PWM_TIM3_SetFreqPreset(PWM_FreqPreset preset)
{
    if (preset >= PWM_FREQ_NUM) return;
    pwmFreqPreset = preset;

    const PWM_FreqConfig *cfg = &pwmFreqTable[preset];

    TIM3->CR1 &= ~TIM_CR1_CEN;     /* 暂停计数器, 安全更新 PSC/ARR */
    TIM3->PSC = cfg->psc;
    TIM3->ARR = cfg->arr;
    TIM3->EGR = TIM_EGR_UG;        /* 立即装载新 PSC/ARR */

    if (pwmEnabled != 0U) pwmApplyCompare();  /* 按新周期重算占空比 */
    else TIM3->CCR4 = 0U;                     /* 关闭时保持恒低 */

    TIM3->CR1 |= TIM_CR1_CEN;      /* 恢复计数 */
}

PWM_FreqPreset PWM_TIM3_GetFreqPreset(void)
{
    return pwmFreqPreset;
}

uint32_t PWM_TIM3_GetFreqHz(void)
{
    return pwmFreqTable[pwmFreqPreset].freqHz;
}

