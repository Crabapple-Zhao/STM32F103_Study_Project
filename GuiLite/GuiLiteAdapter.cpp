/**
 * @file    GuiLiteAdapter.cpp
 * @brief   GuiLite 与 LCD 硬件驱动的桥接层
 *
 * 职责:
 *   1. 实现 EXTERNAL_GFX_OP 回调，将 GuiLite 的 draw_pixel/fill_rect
 *      映射到 LCD 驱动的 LCD_DrawPoint/LCD_Fill
 *   2. 提供 delay_ms() 供 GuiLite 使用
 *
 * 解耦: 本文件是唯一同时了解 GuiLite (EXTERNAL_GFX_OP) 和 LCD 驱动
 *       (LCD_DrawPoint/LCD_Fill) 的模块。GuiLite 层和 LCD 层互不知晓。
 */
#include <stdint.h>
#include "gfx_types.h"

/* C 语言 LCD 驱动接口 */
extern "C" {
#include "lcd.h"
}

/* HAL 延时函数 (C 链接) */
extern "C" void HAL_Delay(uint32_t Delay);

/* ======================== 颜色转换 ======================== */

/* GL_RGB = 0xAARRGGBB → RGB565 */
static unsigned int gl_rgb32_to_16(unsigned int rgb)
{
    return (((rgb >> 19) << 11) & 0xF800) |
           (((rgb >> 10) << 5)  & 0x07E0) |
           ((rgb >> 3) & 0x1F);
}

/* ======================== GuiLite 回调实现 ======================== */

static void gl_draw_pixel(int x, int y, unsigned int rgb)
{
    LCD_DrawPoint((uint16_t)x, (uint16_t)y, (uint16_t)gl_rgb32_to_16(rgb));
}

static void gl_fill_rect(int x0, int y0, int x1, int y1, unsigned int rgb)
{
    LCD_Fill((uint16_t)x0, (uint16_t)y0, (uint16_t)x1, (uint16_t)y1,
             (uint16_t)gl_rgb32_to_16(rgb));
}

/* ======================== 全局 GFX 接口实例 ========================
 * GuiLite c_display 构造函数通过此指针调用底层绘制函数。
 * main.cpp 通过 extern 引用此变量。 */
EXTERNAL_GFX_OP gfx_op = { gl_draw_pixel, gl_fill_rect };

/* ======================== 延时函数 ======================== */

extern "C" void delay_ms(unsigned short nms)
{
    HAL_Delay((uint32_t)nms);
}
