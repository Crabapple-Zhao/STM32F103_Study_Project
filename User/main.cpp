/**
 * @file    main.cpp — GuiLite 按键测试 Demo (STM32F103C8T6 + ST7735S 128x160)
 *
 * 硬件: STM32F103C8T6, ST7735S LCD (SPI+DMA), USART1 (PA9/PA10, 115200)
 * 按键: KEY1=PA0, KEY2=PC13, 高电平按下
 * 编译器: ARMCC V5.06 update 7, C++ 模式 (FileType=8)
 *
 * 注意: 中断处理函数(SysTick_Handler/DMA1_Channel3_IRQHandler)定义在 bsp_init.c 中,
 *       避免C++名称修饰导致链接器移除。
 */
#include "main.h"
#include "GuiLite_min.h"
#include "font_ascii_8x16.h"
#include "lcd.h"
#include "key.h"

extern struct EXTERNAL_GFX_OP gfx_op;

#define APP_VERSION  "v0.2.1"

/* 按键状态显示区域 */
#define KEY_DISP_Y  132

static void draw_key_status(c_surface* s, uint8_t k1, uint8_t k2)
{
    /* 清空状态区 */
    s->fill_rect(0, KEY_DISP_Y, LCD_W - 1, LCD_H - 1, GL_RGB(0, 0, 0), Z_ORDER_LEVEL_0);

    /* KEY1 */
    unsigned int bg1 = k1 ? GL_RGB(0, 180, 0) : GL_RGB(60, 60, 60);
    unsigned int fg1 = k1 ? GL_RGB(255, 255, 255) : GL_RGB(150, 150, 150);
    s->fill_rect(2, KEY_DISP_Y + 2, 62, KEY_DISP_Y + 26, bg1, Z_ORDER_LEVEL_0);
    c_word::draw_string(s, Z_ORDER_LEVEL_0, "KEY1", 6, KEY_DISP_Y + 7,
        &font_8x16_info, fg1, GL_ARGB(0, 0, 0, 0));

    /* KEY2 */
    unsigned int bg2 = k2 ? GL_RGB(0, 100, 200) : GL_RGB(60, 60, 60);
    unsigned int fg2 = k2 ? GL_RGB(255, 255, 255) : GL_RGB(150, 150, 150);
    s->fill_rect(65, KEY_DISP_Y + 2, 125, KEY_DISP_Y + 26, bg2, Z_ORDER_LEVEL_0);
    c_word::draw_string(s, Z_ORDER_LEVEL_0, "KEY2", 69, KEY_DISP_Y + 7,
        &font_8x16_info, fg2, GL_ARGB(0, 0, 0, 0));
}

int main(void)
{
    BSP_Init();

    /* 初始化字体 */
    font_8x16_init();

    /* 创建 GuiLite 显示对象 */
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
    c_word::draw_string(surface, Z_ORDER_LEVEL_0, APP_VERSION, 4, 62,
        &font_8x16_info, GL_RGB(255, 220, 0), GL_ARGB(0, 0, 0, 0));

    /* 彩色条 */
    surface->fill_rect(4, 82, 123, 97,  GL_RGB(255, 0, 0),   Z_ORDER_LEVEL_0);
    surface->fill_rect(4, 99, 41, 114,  GL_RGB(0, 255, 0),   Z_ORDER_LEVEL_0);
    surface->fill_rect(43, 99, 84, 114, GL_RGB(0, 0, 255),   Z_ORDER_LEVEL_0);
    surface->fill_rect(86, 99, 123, 114, GL_RGB(255, 255, 0), Z_ORDER_LEVEL_0);

    /* 边框 */
    surface->draw_rect(2, 20, 125, 156, GL_RGB(255, 255, 255), Z_ORDER_LEVEL_0, 1);

    /* 按键提示 */
    c_word::draw_string(surface, Z_ORDER_LEVEL_0, "Press keys:", 4, 118,
        &font_8x16_info, GL_RGB(180, 180, 180), GL_ARGB(0, 0, 0, 0));

    /* 初始按键状态 */
    uint8_t key1_prev = 0, key2_prev = 0;
    draw_key_status(surface, 0, 0);

    /* 主循环 */
    uint16_t tick = 0;
    while (1)
    {
        uint8_t key1 = KEY_Read(KEY_NUM_1);
        uint8_t key2 = KEY_Read(KEY_NUM_2);

        if (key1 != key1_prev || key2 != key2_prev)
        {
            draw_key_status(surface, key1, key2);
            key1_prev = key1;
            key2_prev = key2;
        }

        /* LED 500ms 闪烁 */
        tick++;
        if (tick >= 10)
        {
            LED_Toggle();
            tick = 0;
        }

        HAL_Delay(50);
    }
}