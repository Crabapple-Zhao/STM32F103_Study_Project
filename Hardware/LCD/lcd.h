/**
 * @file    lcd.h
 * @brief   1.8寸 TFT-LCD (ST7735S, 128x160) 驱动头文件
 * @note    PA5=SCK(SPI1), PA7=MOSI(SPI1), PA4=BLK, PA1=CS, PA2=RST, PA3=DC
 */
#ifndef __LCD_H
#define __LCD_H

#include "stm32f1xx_hal.h"

#define LCD_W  128
#define LCD_H  160

/* ---- 颜色 (RGB565) ---- */
#define COLOR_WHITE   0xFFFF
#define COLOR_BLACK   0x0000
#define COLOR_RED     0xF800
#define COLOR_GREEN   0x07E0
#define COLOR_BLUE    0x001F
#define COLOR_YELLOW  0xFFE0
#define COLOR_MAGENTA 0xF81F
#define COLOR_CYAN    0x07FF

#ifdef __cplusplus
extern "C" {
#endif

void LCD_Init(void);
void LCD_Fill(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);
void LCD_DrawPoint(uint16_t x, uint16_t y, uint16_t color);
void LCD_DrawLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);
void LCD_DrawRect(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);
void LCD_DrawCircle(uint16_t x0, uint16_t y0, uint16_t r, uint16_t color);
void LCD_ShowChar(uint16_t x, uint16_t y, char c, uint16_t fc, uint16_t bc, uint8_t size);
void LCD_ShowString(uint16_t x, uint16_t y, const char *s, uint16_t fc, uint16_t bc, uint8_t size);

/* 逐行写入 RGB565 数据 (供 Astra UI 1bpp→RGB565 刷新用) */
void LCD_WriteLine(uint16_t y, const uint8_t *data, uint16_t len);

/* 流式写入: 单次窗口设置, 连续 DMA 传输整屏 (性能优化) */
void LCD_WriteBegin(void);
void LCD_WriteStreamLine(const uint8_t *data, uint16_t len);
void LCD_WriteEnd(void);

#ifdef __cplusplus
}
#endif

#endif
