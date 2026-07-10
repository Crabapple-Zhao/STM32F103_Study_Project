/**
 * @file    main.cpp — Astra UI 主入口
 */
#include "main.h"
#include "usart.h"
#include "app_config.h"
#include "app_log.h"
#include "hal_port.h"
#include "astra_rocket.h"

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

int main(void)
{
    BSP_Init();
    uart_puts("\r\n[INFO] [boot] firmware=" APP_NAME
              " version=" APP_VERSION
              " ui=" APP_UI_NAME
              " target=" APP_TARGET_NAME
              " lcd=" APP_LCD_NAME "\r\n");
    logMemoryUsage();
    astraHalInit();
    astraCoreInit();

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
