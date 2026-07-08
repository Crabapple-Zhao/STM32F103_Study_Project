/**
 * @file    main.cpp — GuiLite 移植测试 (STM32F103C8T6 + ST7735S 128x160)
 *
 * 硬件: STM32F103C8T6, ST7735S LCD (SPI+DMA), USART1 (PA9/PA10, 115200)
 * 编译器: ARMCC V5.06 update 7, C++ 模式 (FileType=8)
 *
 * 注意: 中断处理函数(SysTick_Handler/DMA1_Channel3_IRQHandler)定义在 bsp_init.c 中,
 *       避免C++名称修饰导致链接器移除。
 */
#include "main.h"
#include "GuiLite_min.h"
#include "font_ascii_8x16.h"
#include "lcd.h"          /* LCD_W, LCD_H, COLOR_BLACK 等宏 */

extern struct EXTERNAL_GFX_OP gfx_op;

int main(void)
{
    BSP_Init();

    /* 初始化字体 */
    font_8x16_init();

    /* 创建 GuiLite 显示对象 (无帧缓冲, 外部GFX回调) */
    c_display display(0, LCD_W, LCD_H, LCD_W, LCD_H, 2, 1, &gfx_op);
    c_surface* surface = display.alloc_surface(Z_ORDER_LEVEL_0);
    c_theme::add_font(FONT_DEFAULT, &font_8x16_info);

    /* 清屏 + 绘制 UI */
    LCD_Fill(0, 0, LCD_W - 1, LCD_H - 1, COLOR_BLACK);

    /* 标题栏 */
    surface->fill_rect(0, 0, LCD_W - 1, 18, GL_RGB(44, 62, 80), Z_ORDER_LEVEL_0);
    c_word::draw_string(surface, Z_ORDER_LEVEL_0, "GuiLite STM32", 2, 2,
        &font_8x16_info, GL_RGB(255, 255, 255), GL_ARGB(0, 0, 0, 0));

    /* 信息行 */
    c_word::draw_string(surface, Z_ORDER_LEVEL_0, "STM32F103C8T6", 4, 26,
        &font_8x16_info, GL_RGB(100, 200, 255), GL_ARGB(0, 0, 0, 0));
    c_word::draw_string(surface, Z_ORDER_LEVEL_0, "ST7735S 128x160", 4, 44,
        &font_8x16_info, GL_RGB(100, 255, 150), GL_ARGB(0, 0, 0, 0));
    c_word::draw_string(surface, Z_ORDER_LEVEL_0, "GuiLite v3.4 OK", 4, 62,
        &font_8x16_info, GL_RGB(255, 220, 0), GL_ARGB(0, 0, 0, 0));

    /* 彩色条 */
    surface->fill_rect(4, 82, 123, 97,  GL_RGB(255, 0, 0),   Z_ORDER_LEVEL_0);   /* 红 */
    surface->fill_rect(4, 99, 41, 114,  GL_RGB(0, 255, 0),   Z_ORDER_LEVEL_0);   /* 绿 */
    surface->fill_rect(43, 99, 84, 114, GL_RGB(0, 0, 255),   Z_ORDER_LEVEL_0);   /* 蓝 */
    surface->fill_rect(86, 99, 123, 114, GL_RGB(255, 255, 0), Z_ORDER_LEVEL_0);   /* 黄 */

    /* 边框 */
    surface->draw_rect(2, 20, 125, 126, GL_RGB(255, 255, 255), Z_ORDER_LEVEL_0, 1);

    /* 主循环 */
    while (1)
    {
        LED_Toggle();
        HAL_Delay(500);
    }
}
