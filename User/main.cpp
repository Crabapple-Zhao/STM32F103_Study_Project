/**
 * @file    main.cpp — GuiLite 按键 + 编码器测试 Demo (STM32F103C8T6 + ST7735S 128x160)
 *
 * 硬件: STM32F103C8T6, ST7735S LCD (SPI+DMA), USART1 (PA9/PA10, 115200)
 * 按键: KEY1=PA0, KEY2=PC13, 高电平按下
 * 编码器: A=PB6(TIM4_CH1), B=PB7(TIM4_CH2), SW=PB5, C=GND
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
#include "encoder.h"
#include <stdio.h>

extern struct EXTERNAL_GFX_OP gfx_op;

#define APP_VERSION  "v0.2.2"

/* 编码器显示区域 */
#define ENC_DISP_Y  116

/* 按键状态显示区域 (缩小后) */
#define KEY_DISP_Y  134

/* 局部刷新: 只重绘单个按键, 不影响另一个 */
static void draw_key1(c_surface* s, uint8_t k1)
{
    unsigned int bg = k1 ? GL_RGB(0, 180, 0) : GL_RGB(60, 60, 60);
    unsigned int fg = k1 ? GL_RGB(255, 255, 255) : GL_RGB(150, 150, 150);
    s->fill_rect(2, KEY_DISP_Y + 1, 62, KEY_DISP_Y + 16, bg, Z_ORDER_LEVEL_0);
    c_word::draw_string(s, Z_ORDER_LEVEL_0, "KEY1", 6, KEY_DISP_Y + 2,
        &font_8x16_info, fg, GL_ARGB(0, 0, 0, 0));
}

static void draw_key2(c_surface* s, uint8_t k2)
{
    unsigned int bg = k2 ? GL_RGB(0, 100, 200) : GL_RGB(60, 60, 60);
    unsigned int fg = k2 ? GL_RGB(255, 255, 255) : GL_RGB(150, 150, 150);
    s->fill_rect(65, KEY_DISP_Y + 1, 125, KEY_DISP_Y + 16, bg, Z_ORDER_LEVEL_0);
    c_word::draw_string(s, Z_ORDER_LEVEL_0, "KEY2", 69, KEY_DISP_Y + 2,
        &font_8x16_info, fg, GL_ARGB(0, 0, 0, 0));
}

/* 局部刷新: 编码器数值区 (x=4~75) 和 SW 区 (x=76~124) 独立更新 */
/* 用不透明背景画字一步完成, 消除 fill_rect+draw_string 两步造成的闪烁 */
static void draw_enc_count(c_surface* s, int16_t count)
{
    char buf[16];
    snprintf(buf, sizeof(buf), "ENC:%+05d", count);
    c_word::draw_string(s, Z_ORDER_LEVEL_0, buf, 4, ENC_DISP_Y + 1,
        &font_8x16_info, GL_RGB(255, 200, 100), GL_RGB(0, 0, 0));
}

static void draw_enc_sw(c_surface* s, uint8_t sw)
{
    const char* sw_text = sw ? "ON " : "OFF";
    unsigned int sw_color = sw ? GL_RGB(255, 100, 100) : GL_RGB(150, 150, 150);
    /* "SW:" 和状态值都用不透明黑背景, 覆盖旧内容 */
    c_word::draw_string(s, Z_ORDER_LEVEL_0, "SW:", 76, ENC_DISP_Y + 1,
        &font_8x16_info, GL_RGB(200, 200, 200), GL_RGB(0, 0, 0));
    c_word::draw_string(s, Z_ORDER_LEVEL_0, sw_text, 100, ENC_DISP_Y + 1,
        &font_8x16_info, sw_color, GL_RGB(0, 0, 0));
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

    /* 初始状态: 全部绘制一次 */
    draw_enc_count(surface, 0);
    draw_enc_sw(surface, 0);
    draw_key1(surface, 0);
    draw_key2(surface, 0);

    uint8_t  key1_prev = 0, key2_prev = 0;
    int16_t  enc_prev  = 0;
    uint8_t  sw_prev   = 0;

    /* 主循环 */
    uint16_t tick = 0;
    while (1)
    {
        /* 按键扫描: 独立判断, 只刷新变化的那个 */
        uint8_t key1 = KEY_Read(KEY_NUM_1);
        uint8_t key2 = KEY_Read(KEY_NUM_2);

        if (key1 != key1_prev)
        {
            draw_key1(surface, key1);
            key1_prev = key1;
        }
        if (key2 != key2_prev)
        {
            draw_key2(surface, key2);
            key2_prev = key2;
        }

        /* 编码器扫描: 数值和 SW 独立刷新 */
        int16_t enc_count = Encoder_GetCount();
        uint8_t sw = Encoder_SW_Read();

        if (enc_count != enc_prev)
        {
            draw_enc_count(surface, enc_count);
            enc_prev = enc_count;
        }
        if (sw != sw_prev)
        {
            draw_enc_sw(surface, sw);
            sw_prev = sw;
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
