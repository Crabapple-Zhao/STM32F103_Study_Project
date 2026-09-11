/**
 * @file    pwm.h — TIM3_CH4 PWM 输出驱动
 * @note    PB1=TIM3_CH4, 预设频率档位 (100Hz/1kHz/10kHz/100kHz), 寄存器级配置
 *          PSC 固定 71 (1MHz 计数), ARR 随档位变化
 */
#ifndef __PWM_H
#define __PWM_H

#include "stm32f1xx_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 预设频率档位 (PSC=71 固定, ARR 决定频率) */
typedef enum {
    PWM_FREQ_100HZ = 0,   /* ARR=9999, 占空比精度 0.01% */
    PWM_FREQ_1KHZ,        /* ARR=999,  占空比精度 0.1%  */
    PWM_FREQ_10KHZ,       /* ARR=99,   占空比精度 1%    */
    PWM_FREQ_100KHZ,      /* ARR=9,    占空比精度 10%   */
    PWM_FREQ_NUM
} PWM_FreqPreset;

/* 初始化 TIM3_CH4 @ PB1, 默认 1kHz, 输出默认关闭 (恒低电平) */
void PWM_TIM3_Init(void);

/* 释放 TIM3 与 PB1 (计数器停止, 时钟关闭, 引脚恢复浮空输入) */
void PWM_TIM3_DeInit(void);

/* 设置占空比 0~100 (%), 输出使能期间立即生效 */
void PWM_TIM3_SetDuty(uint8_t dutyPercent);

/* 使能/关闭输出: 关闭时引脚恒为低电平 (0V), 关闭时保持占空比设定 */
void PWM_TIM3_SetEnable(uint8_t on);

/* 切换频率档位, 保持当前占空比百分比 (在精度允许范围内) 与使能状态 */
void PWM_TIM3_SetFreqPreset(PWM_FreqPreset preset);

/* 获取当前频率档位 */
PWM_FreqPreset PWM_TIM3_GetFreqPreset(void);

/* 获取当前档位对应频率值 (Hz), 用于界面显示 */
uint32_t PWM_TIM3_GetFreqHz(void);

#ifdef __cplusplus
}
#endif

#endif /* __PWM_H */
