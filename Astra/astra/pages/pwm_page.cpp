/**
 * @file    pwm_page.cpp
 * @brief   PWM output submenu: Control (duty + on/off) and Frequency (preset).
 */
#include "pwm_page.h"

#include "app_log.h"
#include "hal.h"
#include "pwm.h"
#include "ui/element/page/item.h"

#include <cstddef>

/* 占空比与频率档位跨页面保留 */
static uint8_t pwmDuty = 50U;
static uint8_t pwmFreqIndex = PWM_FREQ_1KHZ;
static bool pwmRunning = false;

static const char *pwmFreqLabels[PWM_FREQ_NUM] = {
    "100Hz", "1kHz", "10kHz", "100kHz"
};

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

/* 电压显示: 厘米电压 (0~330 -> 0.00~3.30V) */
static void appendVolt2(char *buf, int *pos, uint32_t centiVolt)
{
    appendUInt(buf, pos, centiVolt / 100U);
    buf[(*pos)++] = '.';
    buf[(*pos)++] = (char)('0' + (centiVolt / 10U) % 10U);
    buf[(*pos)++] = (char)('0' + centiVolt % 10U);
}

/* 占空比 -> 平均电压 (厘伏, 四舍五入): duty * 330 / 100 */
static uint32_t dutyToCentiVolt(uint8_t duty, bool running)
{
    if (!running) return 0U;
    return ((uint32_t)duty * 330U + 50U) / 100U;
}

static void logPwmOutput(bool on)
{
    char buf[72];
    int pos = 0;
    appendText(buf, &pos, "[INFO] [pwm] output=");
    appendText(buf, &pos, on ? "on" : "off");
    appendText(buf, &pos, " duty=");
    appendUInt(buf, &pos, pwmDuty);
    appendText(buf, &pos, "% freq=");
    appendText(buf, &pos, pwmFreqLabels[pwmFreqIndex]);
    appendText(buf, &pos, "\r\n");
    buf[pos] = 0;
    uart_puts(buf);
}

/* ===================== 控制界面 ===================== */

static void controlEnter(void)
{
    PWM_TIM3_Init();
    PWM_TIM3_SetFreqPreset((PWM_FreqPreset)pwmFreqIndex);
    PWM_TIM3_SetDuty(pwmDuty);
    PWM_TIM3_SetEnable(0U);  /* 进入不自动输出, 短按旋钮后才开启 */
    pwmRunning = false;

    char buf[96];
    int pos = 0;
    appendText(buf, &pos, "[INFO] [pwm] control_enter pin=PB1 timer=TIM3_CH4 freq=");
    appendText(buf, &pos, pwmFreqLabels[pwmFreqIndex]);
    appendText(buf, &pos, " duty=");
    appendUInt(buf, &pos, pwmDuty);
    appendText(buf, &pos, "%\r\n");
    buf[pos] = 0;
    uart_puts(buf);
}

static void controlExit(void)
{
    PWM_TIM3_DeInit();
    pwmRunning = false;
    APP_LOG_INFO("pwm", "control_exit released=1 timer=TIM3");
}

/* 旋钮=调占空比(+/-1%), 短按=开关输出, 长按不消费(返回上级) */
static bool controlKey(key::KEY_INDEX idx, key::KEY_ACTION act)
{
    if (act == key::CLICK) {
        if (idx == key::KEY_1) {       /* CW: +1% */
            if (pwmDuty < 100U) pwmDuty++;
        } else {                       /* CCW: -1% */
            if (pwmDuty > 0U) pwmDuty--;
        }
        PWM_TIM3_SetDuty(pwmDuty);
        return true;
    }
    if (act == key::PRESS && idx == key::KEY_1) {  /* 短按: 输出开关 */
        pwmRunning = !pwmRunning;
        PWM_TIM3_SetEnable(pwmRunning ? 1U : 0U);
        logPwmOutput(pwmRunning);
        return true;
    }
    return false;  /* 长按等其余事件走默认导航 (返回上级) */
}

static void drawControl(void)
{
    drawCenteredText("PWM Control", 34);
    HAL::drawHLine((APP_DISPLAY_WIDTH - 108) / 2, 42, 108);

    drawCenteredText(pwmRunning ? "Output: ON" : "Output: OFF", 58);

    /* 占空比 */
    char dutyBuf[16];
    int pos = 0;
    appendText(dutyBuf, &pos, "Duty: ");
    appendUInt(dutyBuf, &pos, pwmDuty);
    appendText(dutyBuf, &pos, "%");
    dutyBuf[pos] = 0;
    drawCenteredText(dutyBuf, 76);

    /* 占空比实时进度条: 外框 100px, 内部按比例填充 */
    HAL::drawFrame((APP_DISPLAY_WIDTH - 100) / 2, 80, 100, 8);
    HAL::drawBox((APP_DISPLAY_WIDTH - 96) / 2, 82, (float)((uint32_t)96U * pwmDuty / 100U), 4);

    /* 平均电压 / 最大电压 / 当前频率 */
    char voltBuf[28];
    pos = 0;
    appendVolt2(voltBuf, &pos, dutyToCentiVolt(pwmDuty, pwmRunning));
    appendText(voltBuf, &pos, "/3.30V ");
    appendText(voltBuf, &pos, pwmFreqLabels[pwmFreqIndex]);
    voltBuf[pos] = 0;
    drawCenteredText(voltBuf, 106);
}

/* ===================== 频率设置界面 ===================== */

static void freqEnter(void)
{
    char buf[64];
    int pos = 0;
    appendText(buf, &pos, "[INFO] [pwm] freq_enter freq=");
    appendText(buf, &pos, pwmFreqLabels[pwmFreqIndex]);
    appendText(buf, &pos, "\r\n");
    buf[pos] = 0;
    uart_puts(buf);
}

static void freqExit(void)
{
    /* 频率只改全局档位, 不占用 TIM3, 无需释放资源 */
}

/* 旋钮=切换档位, 短按/长按不消费 (长按返回上级) */
static bool freqKey(key::KEY_INDEX idx, key::KEY_ACTION act)
{
    if (act == key::CLICK) {
        if (idx == key::KEY_1) {           /* CW: 下一档 */
            if (pwmFreqIndex < PWM_FREQ_NUM - 1U) pwmFreqIndex++;
        } else {                           /* CCW: 上一档 */
            if (pwmFreqIndex > 0U) pwmFreqIndex--;
        }
        return true;
    }
    return false;
}

static void drawFreq(void)
{
    drawCenteredText("Frequency", 24);
    HAL::drawHLine((APP_DISPLAY_WIDTH - 108) / 2, 32, 108);

    for (uint8_t i = 0; i < PWM_FREQ_NUM; i++) {
        char buf[16];
        int pos = 0;
        appendText(buf, &pos, (i == pwmFreqIndex) ? "> " : "  ");
        appendText(buf, &pos, pwmFreqLabels[i]);
        buf[pos] = 0;
        drawCenteredText(buf, (float)(50 + i * 18));
    }
}

void PwmPage_AddToMenu(astra::Menu *menu)
{
    if (menu == 0) return;

    astra::Menu *sub = new astra::Menu("-PWM-Output");
    sub->addItem(new astra::Menu("-Control", drawControl,
                                 controlEnter, controlExit, controlKey));
    sub->addItem(new astra::Menu("-Frequency", drawFreq,
                                 freqEnter, freqExit, freqKey));
    menu->addItem(sub);
}
