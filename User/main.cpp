/**
 * @file    main.cpp — Astra UI 主入口
 */
#include "main.h"
#include "usart.h"
#include "hal_port.h"
#include "astra_rocket.h"

int main(void)
{
    BSP_Init();
    uart_puts("\r\n=== Dev-Beta v0.3.1 (Astra UI) ===\r\n");
    astraHalInit();
    astraCoreInit();

    uart_puts("[main] astra started\r\n");

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
            const char prefix[] = "[main] fps=";
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
