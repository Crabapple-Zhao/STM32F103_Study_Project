/**
 * @file    lcd.c — ST7735S HAL SPI1 + DMA
 *          PA4=BLK, PA1=CS, PA2=RST, PA3=DC, PA5=SCK, PA7=MOSI
 */
#include "lcd.h"
#include "spi.h"
#include "dma.h"
#include "lcdfont.h"

#define P_BLK GPIO_PIN_4
#define P_CS  GPIO_PIN_1
#define P_RST GPIO_PIN_2
#define P_DC  GPIO_PIN_3

#define BLK_HI() HAL_GPIO_WritePin(GPIOA, P_BLK, GPIO_PIN_SET)
#define CS_LO()  HAL_GPIO_WritePin(GPIOA, P_CS,  GPIO_PIN_RESET)
#define CS_HI()  HAL_GPIO_WritePin(GPIOA, P_CS,  GPIO_PIN_SET)
#define RST_LO() HAL_GPIO_WritePin(GPIOA, P_RST, GPIO_PIN_RESET)
#define RST_HI() HAL_GPIO_WritePin(GPIOA, P_RST, GPIO_PIN_SET)
#define DC_LO()  HAL_GPIO_WritePin(GPIOA, P_DC,  GPIO_PIN_RESET)
#define DC_HI()  HAL_GPIO_WritePin(GPIOA, P_DC,  GPIO_PIN_SET)

static void lcd_cmd(uint8_t c)  { DC_LO(); CS_LO(); SPI1_WriteByte(c); CS_HI(); DC_HI(); }
static void lcd_data8(uint8_t d) { CS_LO(); SPI1_WriteByte(d); CS_HI(); }
static void lcd_data16(uint16_t d) { CS_LO(); SPI1_WriteByte(d>>8); SPI1_WriteByte(d); CS_HI(); }

static void lcd_window(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2)
{
    lcd_cmd(0x2A); lcd_data16(x1); lcd_data16(x2);
    lcd_cmd(0x2B); lcd_data16(y1); lcd_data16(y2);
    lcd_cmd(0x2C);
}

void LCD_Init(void)
{
    __HAL_RCC_GPIOA_CLK_ENABLE();
    GPIO_InitTypeDef g = {0};
    g.Pin   = P_BLK | P_CS | P_RST | P_DC;
    g.Mode  = GPIO_MODE_OUTPUT_PP;
    g.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &g);
    HAL_GPIO_WritePin(GPIOA, P_BLK | P_CS | P_RST | P_DC, GPIO_PIN_SET);

    SPI1_Init();
    DMA_Setup();

    RST_LO(); HAL_Delay(100); RST_HI(); HAL_Delay(100);
    BLK_HI(); HAL_Delay(50);

    lcd_cmd(0x11); HAL_Delay(120);
    lcd_cmd(0xB1); lcd_data8(0x05); lcd_data8(0x3C); lcd_data8(0x3C);
    lcd_cmd(0xB2); lcd_data8(0x05); lcd_data8(0x3C); lcd_data8(0x3C);
    lcd_cmd(0xB3); lcd_data8(0x05); lcd_data8(0x3C); lcd_data8(0x3C);
    lcd_data8(0x05); lcd_data8(0x3C); lcd_data8(0x3C);
    lcd_cmd(0xB4); lcd_data8(0x03);
    lcd_cmd(0xC0); lcd_data8(0x28); lcd_data8(0x08); lcd_data8(0x04);
    lcd_cmd(0xC1); lcd_data8(0xC0);
    lcd_cmd(0xC2); lcd_data8(0x0D); lcd_data8(0x00);
    lcd_cmd(0xC3); lcd_data8(0x8D); lcd_data8(0x2A);
    lcd_cmd(0xC4); lcd_data8(0x8D); lcd_data8(0xEE);
    lcd_cmd(0xC5); lcd_data8(0x1A);
    lcd_cmd(0x36); lcd_data8(0xC0);
    lcd_cmd(0xE0);
    lcd_data8(0x04);lcd_data8(0x22);lcd_data8(0x07);lcd_data8(0x0A);
    lcd_data8(0x2E);lcd_data8(0x30);lcd_data8(0x25);lcd_data8(0x2A);
    lcd_data8(0x28);lcd_data8(0x26);lcd_data8(0x2E);lcd_data8(0x3A);
    lcd_data8(0x00);lcd_data8(0x01);lcd_data8(0x03);lcd_data8(0x13);
    lcd_cmd(0xE1);
    lcd_data8(0x04);lcd_data8(0x16);lcd_data8(0x06);lcd_data8(0x0D);
    lcd_data8(0x2D);lcd_data8(0x26);lcd_data8(0x23);lcd_data8(0x27);
    lcd_data8(0x27);lcd_data8(0x25);lcd_data8(0x2D);lcd_data8(0x3B);
    lcd_data8(0x00);lcd_data8(0x01);lcd_data8(0x04);lcd_data8(0x13);
    lcd_cmd(0x3A); lcd_data8(0x05);
    lcd_cmd(0x29);
}

void LCD_Fill(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color)
{
    static uint16_t dma_color;  /* DMA 必须从静态/全局地址读取，栈变量可能被覆盖 */
    uint32_t n = (uint32_t)(x2 - x1 + 1) * (y2 - y1 + 1);
    dma_color = color;
    lcd_window(x1, y1, x2, y2);
    CS_LO();
    SPI1->CR1 |= SPI_CR1_DFF;
    DMA_SPI1_Tx16(&dma_color, (uint16_t)n);
    SPI1->CR1 &= ~SPI_CR1_DFF;
    CS_HI();
}

void LCD_DrawPoint(uint16_t x, uint16_t y, uint16_t color)
{
    lcd_window(x, y, x, y);
    lcd_data16(color);
}

void LCD_DrawLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color)
{
    int dx = x2 - x1, dy = y2 - y1;
    int sx = dx > 0 ? 1 : (dx < 0 ? -1 : 0);
    int sy = dy > 0 ? 1 : (dy < 0 ? -1 : 0);
    int err, e2;
    if (dx < 0) dx = -dx;
    if (dy < 0) dy = -dy;
    err = (dx > dy ? dx : -dy) / 2;
    for (;;) {
        LCD_DrawPoint(x1, y1, color);
        if (x1 == x2 && y1 == y2) break;
        e2 = err;
        if (e2 > -dx) { err -= dy; x1 += sx; }
        if (e2 <  dy) { err += dx; y1 += sy; }
    }
}

void LCD_DrawRect(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color)
{
    LCD_DrawLine(x1,y1,x2,y1,color); LCD_DrawLine(x2,y1,x2,y2,color);
    LCD_DrawLine(x2,y2,x1,y2,color); LCD_DrawLine(x1,y2,x1,y1,color);
}

void LCD_DrawCircle(uint16_t x0, uint16_t y0, uint16_t r, uint16_t color)
{
    int a = 0, b = r;
    while (a <= b) {
        LCD_DrawPoint(x0+b,y0+a,color); LCD_DrawPoint(x0+a,y0+b,color);
        LCD_DrawPoint(x0-a,y0+b,color); LCD_DrawPoint(x0-b,y0+a,color);
        LCD_DrawPoint(x0-b,y0-a,color); LCD_DrawPoint(x0-a,y0-b,color);
        LCD_DrawPoint(x0+a,y0-b,color); LCD_DrawPoint(x0+b,y0-a,color);
        a++; if (a*a + b*b > r*r) b--;
    }
}

void LCD_ShowChar(uint16_t x, uint16_t y, char c, uint16_t fc, uint16_t bc, uint8_t size)
{
    uint8_t i, b, sizex, t, m = 0;
    uint16_t idx, typeface;
    idx = c - ' ';
    sizex = size / 2;
    typeface = (sizex / 8 + ((sizex % 8) ? 1 : 0)) * size;
    lcd_window(x, y, x + sizex - 1, y + size - 1);
    for (i = 0; i < typeface; i++) {
        if      (size == 12) b = ascii_1206[idx][i];
        else if (size == 16) b = ascii_1608[idx][i];
        else if (size == 24) b = ascii_2412[idx][i];
        else if (size == 32) b = ascii_3216[idx][i];
        else return;
        for (t = 0; t < 8; t++) {
            lcd_data16((b & (0x01 << t)) ? fc : bc);
            m++; if (m % sizex == 0) { m = 0; break; }
        }
    }
}

void LCD_ShowString(uint16_t x, uint16_t y, const char *s, uint16_t fc, uint16_t bc, uint8_t size)
{
    while (*s) { LCD_ShowChar(x, y, *s, fc, bc, size); x += size / 2; s++; }
}

void LCD_WriteLine(uint16_t y, const uint8_t *data, uint16_t len)
{
    lcd_window(0, y, LCD_W - 1, y);
    CS_LO();
    DMA_SPI1_Tx(data, len);
    CS_HI();
}

/* ---- 流式写入: 单次窗口设置, 连续 DMA 传输整屏 ---- */
void LCD_WriteBegin(void)
{
    lcd_window(0, 0, LCD_W - 1, LCD_H - 1);
    CS_LO();
}

void LCD_WriteStreamLine(const uint8_t *data, uint16_t len)
{
    DMA_SPI1_Tx(data, len);
}

void LCD_WriteEnd(void)
{
    CS_HI();
}
