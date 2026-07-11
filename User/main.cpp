/**
 * @file    main.cpp — Astra UI 主入口
 */
#include "main.h"
#include "usart.h"
#include "app_config.h"
#include "app_log.h"
#include "hal_port.h"
#include "astra_rocket.h"

#if defined(__CC_ARM)
extern "C" unsigned int __heap_base;
extern "C" unsigned int __heap_limit;
#endif

static const uint8_t HEAP_PROBE_PATTERN = 0xA5;

static void appendText(char *buf, int *pos, const char *text)
{
    for (int i = 0; text[i]; i++) buf[(*pos)++] = text[i];
}

static void appendUInt(char *buf, int *pos, uint32_t value)
{
    if (value == 0) {
        buf[(*pos)++] = '0';
        return;
    }
    char tmp[10];
    int len = 0;
    while (value > 0) {
        tmp[len++] = '0' + value % 10U;
        value /= 10U;
    }
    while (len > 0) buf[(*pos)++] = tmp[--len];
}

static void logMemoryUsage(void)
{
    AstraMemoryUsage usage;
    astraGetMemoryUsage(&usage);

    char buf[96];
    int p = 0;
    appendText(buf, &p, "[INFO] [memory] ram=");
    appendUInt(buf, &p, usage.ramBytes);
    appendText(buf, &p, " ram_pct=");
    appendUInt(buf, &p, usage.ramPercent);
    appendText(buf, &p, " rom=");
    appendUInt(buf, &p, usage.romBytes);
    appendText(buf, &p, " rom_pct=");
    appendUInt(buf, &p, usage.romPercent);
    appendText(buf, &p, "\r\n");
    buf[p] = 0;
    uart_puts(buf);
}

static void heapProbeInit(void)
{
#if defined(__CC_ARM)
    uint8_t *base = (uint8_t *)&__heap_base;
    uint8_t *limit = (uint8_t *)&__heap_limit;
    while (base < limit) *base++ = HEAP_PROBE_PATTERN;
#endif
}

static uint32_t heapProbeTotal(void)
{
#if defined(__CC_ARM)
    return (uint32_t)((uint8_t *)&__heap_limit - (uint8_t *)&__heap_base);
#else
    return 0;
#endif
}

static uint32_t heapProbeWatermarkUsed(void)
{
#if defined(__CC_ARM)
    uint8_t *base = (uint8_t *)&__heap_base;
    uint8_t *p = (uint8_t *)&__heap_limit;
    while (p > base) {
        p--;
        if (*p != HEAP_PROBE_PATTERN) return (uint32_t)(p - base + 1);
    }
#endif
    return 0;
}

static void logHeapUsage(const char *stage)
{
    uint32_t total = heapProbeTotal();
    uint32_t used = heapProbeWatermarkUsed();
    uint32_t freeBytes = (total > used) ? (total - used) : 0;

    char buf[112];
    int p = 0;
    appendText(buf, &p, "[INFO] [heap] stage=");
    appendText(buf, &p, stage);
    appendText(buf, &p, " total=");
    appendUInt(buf, &p, total);
    appendText(buf, &p, " watermark_used=");
    appendUInt(buf, &p, used);
    appendText(buf, &p, " watermark_free=");
    appendUInt(buf, &p, freeBytes);
    appendText(buf, &p, "\r\n");
    buf[p] = 0;
    uart_puts(buf);
}

int main(void)
{
    BSP_Init();
    heapProbeInit();
    uart_puts("\r\n[INFO] [boot] firmware=" APP_NAME
              " version=" APP_VERSION
              " ui=" APP_UI_NAME
              " target=" APP_TARGET_NAME
              " lcd=" APP_LCD_NAME "\r\n");
    logMemoryUsage();
    logHeapUsage("before_astra");
    astraHalInit();
    astraCoreInit();
    logHeapUsage("after_astra");

    APP_LOG_INFO("main", "astra_started=true");

    uint32_t lastTick = HAL_GetTick();
    uint32_t ledTick = HAL_GetTick();
    uint32_t frameCount = 0;
    while (1)
    {
        astraLoop();
        frameCount++;
        uint32_t now = HAL_GetTick();
        /* LED 心跳: 500ms 翻转 */
        if (now - ledTick >= 500)
        {
            LED_Toggle();
            ledTick = now;
        }
        /* FPS 统计: 2000ms 输出一次 */
        if (now - lastTick >= 2000)
        {
            char buf[48];
            uint32_t fps = frameCount * 1000 / (now - lastTick);
            const char hex[] = "0123456789";
            int p = 0;
            const char prefix[] = "[INFO] [main] fps=";
            for (int i = 0; prefix[i]; i++) buf[p++] = prefix[i];
            if (fps == 0) { buf[p++] = '0'; }
            else {
                char tmp[12]; int t = 0;
                while (fps > 0) { tmp[t++] = hex[fps % 10]; fps /= 10; }
                while (t > 0) buf[p++] = tmp[--t];
            }
            buf[p++] = '\r'; buf[p++] = '\n'; buf[p] = 0;
            uart_puts(buf);
            frameCount = 0;
            lastTick = now;
        }
    }
}
