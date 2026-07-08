//
// HAL 移植实现 — Astra UI → STM32F103C8T6 + ST7735S (128x160 RGB565)
//
// 核心策略：使用 1bpp 虚拟显存 (128*160/8 = 2560 字节)，
// canvasUpdate 时逐行转换为 RGB565 并通过 SPI+DMA 推送到 LCD。
// 编码器旋钮替代物理按键：旋转=上下导航，SW短按=进入，SW长按=返回。
//
#include "hal_port.h"
#include "hal.h"
#include "lcd.h"
#include "dma.h"
#include "encoder.h"
#include "stm32f1xx_hal.h"
#include <cstring>
#include <string.h>
#include <string>

/* 8x16 ASCII 字体 (行优先 1bpp, 每字节=1行8像素, MSB=左像素) */
#include "font_ascii_8x16.h"

/* ---- 1bpp 虚拟显存 ---- */
/* 格式与 SSD1306 一致: byte_index = x + (y/8)*width, bit = 1<<(y%8) */
#define ASTRA_SCREEN_W  128
#define ASTRA_SCREEN_H  160
#define ASTRA_BUF_SIZE  (ASTRA_SCREEN_W * ASTRA_SCREEN_H / 8)  /* 2560 字节 */

static uint8_t canvasBuffer[ASTRA_BUF_SIZE];

/* RGB565 行缓冲 (128 像素 * 2 字节 = 256 字节, static 供 DMA) */
static uint8_t lineBuf[ASTRA_SCREEN_W * 2];

/* 前景色 (1bpp=1) 和背景色 (1bpp=0) 的 RGB565 值 */
#define FG_COLOR  0xFFFF  /* 白色 */
#define BG_COLOR  0x0000  /* 黑色 */

/* ---- 状态栏 ---- */
#define STATUS_BAR_H   16   /* 顶部状态栏高度 (像素) */
#define BOTTOM_BAR_H   32   /* 底部状态栏高度 (像素, 顶部的 2 倍) */
#define UI_OFFSET_Y    16   /* UI 内容在 canvasBuffer 中的 y 偏移 (避开顶部状态栏) */
#define UI_MAX_Y       (ASTRA_SCREEN_H - BOTTOM_BAR_H)  /* UI 内容最大 y 坐标 = 128, 避开底部状态栏 */
static char statusBarTitle[20] = {0};  /* 顶部状态栏标题文本 */

/* ---- 底部状态栏 FPS 计算 ---- */
static uint32_t fpsLastTick = 0;
static uint32_t fpsFrameCount = 0;
static uint8_t  currentFps = 0;

/* 设置状态栏标题 (供 launcher 调用) */
extern "C" void astraSetStatusBarTitle(const char *title) {
  if (title == nullptr) { statusBarTitle[0] = 0; return; }
  int i = 0;
  for (; i < (int)sizeof(statusBarTitle) - 1 && title[i]; i++) statusBarTitle[i] = title[i];
  statusBarTitle[i] = 0;
}

/* ---- 顶部状态栏右侧图标 ---- */
#define ICON_GAP 2

/* 电池 16×10 (横向, 左侧正极凸起, 填充约 80%, 偏右 1px) */
#define BAT_W 16
#define BAT_H 10
static const uint8_t icon_battery[] = {
  /* Row 0 */ 0x00, 0x00,
  /* Row 1: body top (14px, p2~p15) */ 0xFC, 0xFF,
  /* Row 2: nub(p0,p1) + wall(p2) + gap + wall(p15) */ 0x07, 0x80,
  /* Row 3: nub + wall + gap(p3) + fill(p4~p13) + gap(p14) + wall */ 0xF7, 0xBF,
  /* Row 4 */ 0xF7, 0xBF,
  /* Row 5 */ 0xF7, 0xBF,
  /* Row 6 */ 0xF7, 0xBF,
  /* Row 7: nub + wall + gap + wall */ 0x07, 0x80,
  /* Row 8: body bottom */ 0xFC, 0xFF,
  /* Row 9 */ 0x00, 0x00,
};

/* WiFi 8×8 (待优化) */
#define WIFI_W 8
#define WIFI_H 8
static const uint8_t icon_wifi[] = {
  0x18, 0x3C, 0x42, 0x18, 0x00, 0x24, 0x18, 0x00
};

/* SD 卡 8×8 (待优化) */
#define SD_W 8
#define SD_H 8
static const uint8_t icon_sd[] = {
  0x7E, 0x42, 0x42, 0x42, 0x7E, 0x42, 0x42, 0x7E
};

/* 绘制小型图标到 canvasBuffer (LSB-first, 直接写显存, 支持多字节行宽) */
static void drawIcon(int x, int y, const uint8_t *data, int w, int h) {
  int bytesPerRow = (w + 7) / 8;
  for (int row = 0; row < h; row++) {
    for (int col = 0; col < w; col++) {
      int byteIdx = row * bytesPerRow + col / 8;
      int bitIdx  = col % 8;
      if (data[byteIdx] & (1 << bitIdx)) {
        int px = x + col, py = y + row;
        if (px >= 0 && px < ASTRA_SCREEN_W && py >= 0 && py < ASTRA_SCREEN_H) {
          canvasBuffer[px + (py / 8) * ASTRA_SCREEN_W] |= (1 << (py % 8));
        }
      }
    }
  }
}

/* 在 canvasBuffer 顶部绘制状态栏 (y=0~STATUS_BAR_H-1) */
static void drawStatusBar() {
  /* 1. 清除状态栏区域 */
  for (int y = 0; y < STATUS_BAR_H; y++) {
    uint8_t *row = &canvasBuffer[(y / 8) * ASTRA_SCREEN_W];
    uint8_t bit = 1 << (y % 8);
    uint8_t nb = ~bit;
    for (int x = 0; x < ASTRA_SCREEN_W; x++) row[x] &= nb;
  }

  /* 2. 画标题文字 (左侧, 8x16 字体, y=0~15) */
  for (size_t i = 0; i < sizeof(statusBarTitle) && statusBarTitle[i]; i++) {
    char c = statusBarTitle[i];
    if (c < ' ' || c > '~') c = ' ';
    const unsigned char *glyph = font_8x16_data[c - ' '];
    for (int row = 0; row < 16; row++) {
      unsigned char b = glyph[row];
      for (int col = 0; col < 8; col++) {
        if (b & (0x80 >> col)) {
          int x = (int)(i * 8 + col);
          int y = row;
          if (x >= 0 && x < ASTRA_SCREEN_W && y >= 0 && y < STATUS_BAR_H) {
            canvasBuffer[x + (y / 8) * ASTRA_SCREEN_W] |= (1 << (y % 8));
          }
        }
      }
    }
  }

  /* 3. 右侧静态图标: WiFi(8×8) | SD卡(8×8) | 电池(16×10) */
  {
    int iconY;
    int iconX = ASTRA_SCREEN_W - ICON_GAP - BAT_W;
    iconY = (STATUS_BAR_H - BAT_H) / 2;
    drawIcon(iconX, iconY, icon_battery, BAT_W, BAT_H);
    iconX -= ICON_GAP + SD_W;
    iconY = (STATUS_BAR_H - SD_H) / 2;
    drawIcon(iconX, iconY, icon_sd, SD_W, SD_H);
    iconX -= ICON_GAP + WIFI_W;
    iconY = (STATUS_BAR_H - WIFI_H) / 2;
    drawIcon(iconX, iconY, icon_wifi, WIFI_W, WIFI_H);
  }

  /* 4. 画底部分隔线 (y=STATUS_BAR_H-1) */
  {
    int y = STATUS_BAR_H - 1;
    uint8_t *row = &canvasBuffer[(y / 8) * ASTRA_SCREEN_W];
    uint8_t bit = 1 << (y % 8);
    for (int x = 0; x < ASTRA_SCREEN_W; x++) row[x] |= bit;
  }
}

/* 在 canvasBuffer 顶部绘制一个 8x16 字符 (直接写显存, 不经过 UI_OFFSET_Y/UI_MAX_Y) */
static void drawCharDirect(int x, int y, char c) {
  if (c < ' ' || c > '~') c = ' ';
  const unsigned char *glyph = font_8x16_data[c - ' '];
  for (int row = 0; row < 16; row++) {
    unsigned char b = glyph[row];
    for (int col = 0; col < 8; col++) {
      if (b & (0x80 >> col)) {
        int px = x + col, py = y + row;
        if (px >= 0 && px < ASTRA_SCREEN_W && py >= 0 && py < ASTRA_SCREEN_H) {
          canvasBuffer[px + (py / 8) * ASTRA_SCREEN_W] |= (1 << (py % 8));
        }
      }
    }
  }
}

/* 在 canvasBuffer 底部绘制状态栏 (y=UI_MAX_Y ~ ASTRA_SCREEN_H-1, 32px 高) */
static void drawBottomStatusBar() {
  /* 1. 清除底部状态栏区域 (y=128~159) */
  for (int y = UI_MAX_Y; y < ASTRA_SCREEN_H; y++) {
    uint8_t *row = &canvasBuffer[(y / 8) * ASTRA_SCREEN_W];
    uint8_t bit = 1 << (y % 8);
    uint8_t nb = ~bit;
    for (int x = 0; x < ASTRA_SCREEN_W; x++) row[x] &= nb;
  }

  /* 2. 画顶部分隔线 (y=UI_MAX_Y) */
  {
    int y = UI_MAX_Y;
    uint8_t *row = &canvasBuffer[(y / 8) * ASTRA_SCREEN_W];
    uint8_t bit = 1 << (y % 8);
    for (int x = 0; x < ASTRA_SCREEN_W; x++) row[x] |= bit;
  }

  /* 3. 第一行文字 (y=UI_MAX_Y+1 ~ UI_MAX_Y+16, 即 129~144): 左侧版本, 右侧 FPS */
  {
    const char *ver = "v0.3.4";
    int x = 0;
    for (int i = 0; ver[i]; i++) { drawCharDirect(x, UI_MAX_Y + 1, ver[i]); x += 8; }

    /* FPS 右对齐: "FPS:XX" 共 6 字符 = 48 像素 */
    char fpsBuf[8];
    fpsBuf[0] = 'F'; fpsBuf[1] = 'P'; fpsBuf[2] = 'S'; fpsBuf[3] = ':';
    uint8_t fps = currentFps;  /* 用局部变量, 避免修改全局 */
    if (fps == 0) { fpsBuf[4] = '0'; fpsBuf[5] = 0; }
    else {
      fpsBuf[5] = '0' + fps % 10; fps /= 10;
      fpsBuf[4] = (fps > 0) ? ('0' + fps) : ' ';
      fpsBuf[6] = 0;
    }
    int fpsLen = 0;
    while (fpsBuf[fpsLen]) fpsLen++;
    int fpsX = ASTRA_SCREEN_W - fpsLen * 8;
    for (int i = 0; fpsBuf[i]; i++) { drawCharDirect(fpsX, UI_MAX_Y + 1, fpsBuf[i]); fpsX += 8; }
  }

  /* 4. 第二行文字 (y=UI_MAX_Y+17 ~ UI_MAX_Y+32, 即 145~160→裁剪到159): 运行时间 HH:MM:SS */
  {
    /* 计算时分秒: 总秒数 = HAL_GetTick()/1000 */
    uint32_t secTotal = HAL_GetTick() / 1000;
    uint32_t hours   = secTotal / 3600;
    uint32_t minutes = (secTotal % 3600) / 60;
    uint32_t seconds = secTotal % 60;

    /* 格式: HH:MM:SS (8 字符 = 64px), 左对齐填满整行 */
    char timeBuf[10];
    timeBuf[0] = '0' + (hours / 10);
    timeBuf[1] = '0' + (hours % 10);
    timeBuf[2] = ':';
    timeBuf[3] = '0' + (minutes / 10);
    timeBuf[4] = '0' + (minutes % 10);
    timeBuf[5] = ':';
    timeBuf[6] = '0' + (seconds / 10);
    timeBuf[7] = '0' + (seconds % 10);
    timeBuf[8] = 0;

    int x = 0;
    for (int i = 0; timeBuf[i]; i++) {
      if (x + 8 > ASTRA_SCREEN_W) break;
      drawCharDirect(x, UI_MAX_Y + 17, timeBuf[i]);
      x += 8;
    }
  }
}

class AstraHALPort : public HAL {
public:
  std::string type() override { return "STM32F103_ST7735S"; }

  /* ---- 画布缓冲 ---- */
  void *_getCanvasBuffer() override { return canvasBuffer; }
  uint8_t _getBufferTileHeight() override { return ASTRA_SCREEN_H / 8; }  /* 20 */
  uint8_t _getBufferTileWidth() override { return ASTRA_SCREEN_W; }       /* 128 */

  /* ---- 画布刷新: 1bpp → RGB565 → LCD (流式写入, 单次窗口设置) ---- */
  void _canvasUpdate() override {
    /* 先在 canvasBuffer 顶部绘制状态栏 (覆盖 UI 在该区域的残留) */
    drawStatusBar();
    /* 再在 canvasBuffer 底部绘制状态栏 (覆盖 UI 在该区域的残留) */
    drawBottomStatusBar();

    /* FPS 计算 (基于 canvasUpdate 调用次数) */
    fpsFrameCount++;
    uint32_t now = HAL_GetTick();
    if (now - fpsLastTick >= 1000) {
      currentFps = (uint8_t)(fpsFrameCount * 1000 / (now - fpsLastTick));
      fpsFrameCount = 0;
      fpsLastTick = now;
    }

    LCD_WriteBegin();
    for (int y = 0; y < ASTRA_SCREEN_H; y++) {
      uint8_t bit = 1 << (y % 8);
      const uint8_t *row = &canvasBuffer[(y / 8) * ASTRA_SCREEN_W];
      for (int x = 0; x < ASTRA_SCREEN_W; x++) {
        uint16_t color = (row[x] & bit) ? FG_COLOR : BG_COLOR;
        lineBuf[x * 2]     = color >> 8;
        lineBuf[x * 2 + 1] = color & 0xFF;
      }
      LCD_WriteStreamLine(lineBuf, ASTRA_SCREEN_W * 2);
    }
    LCD_WriteEnd();
  }

  void _canvasClear() override {
    memset(canvasBuffer, 0, ASTRA_BUF_SIZE);
  }

  /* ---- 字体 ---- */
  void _setFont(const uint8_t *) override {}  /* 使用内置 8x16, 无需外部字体 */

  uint8_t _getFontWidth(std::string &_text) override {
    return (uint8_t)(_text.length() * 8);
  }

  uint8_t _getFontHeight() override { return 16; }

  /* ---- 绘图原语 ---- */
  uint8_t drawType = 1;

  void _setDrawType(uint8_t _type) override { drawType = _type; }

  void _drawPixel(float _x, float _y) override {
    int x = (int)_x, y = (int)_y + UI_OFFSET_Y;
    if (x < 0 || x >= ASTRA_SCREEN_W || y < 0 || y >= UI_MAX_Y) return;
    uint16_t idx = x + (y / 8) * ASTRA_SCREEN_W;
    uint8_t bit = 1 << (y % 8);
    if (drawType == 1) canvasBuffer[idx] |= bit;
    else if (drawType == 2) canvasBuffer[idx] ^= bit;
    else canvasBuffer[idx] &= ~bit;
  }

  /* 优化: 直接字节操作, 避免 float→int 和边界检查开销 */
  void _drawHLine(float _x, float _y, float _l) override {
    int x0 = (int)_x, y0 = (int)_y + UI_OFFSET_Y, l = (int)_l;
    if (l <= 0 || y0 < 0 || y0 >= UI_MAX_Y) return;
    int x1 = x0 + l - 1;
    if (x0 < 0) x0 = 0;
    if (x1 >= ASTRA_SCREEN_W) x1 = ASTRA_SCREEN_W - 1;
    if (x0 > x1) return;
    uint8_t bit = 1 << (y0 % 8);
    uint8_t *row = &canvasBuffer[(y0 / 8) * ASTRA_SCREEN_W];
    if (drawType == 1)      { for (int x = x0; x <= x1; x++) row[x] |= bit; }
    else if (drawType == 2) { for (int x = x0; x <= x1; x++) row[x] ^= bit; }
    else                    { uint8_t nb = ~bit; for (int x = x0; x <= x1; x++) row[x] &= nb; }
  }

  void _drawVLine(float _x, float _y, float _h) override {
    int x0 = (int)_x, y0 = (int)_y + UI_OFFSET_Y, h = (int)_h;
    if (h <= 0 || x0 < 0 || x0 >= ASTRA_SCREEN_W) return;
    int y1 = y0 + h - 1;
    if (y0 < 0) y0 = 0;
    if (y1 >= UI_MAX_Y) y1 = UI_MAX_Y - 1;
    if (y0 > y1) return;
    int page = y0 / 8;
    int bitIdx = y0 % 8;
    uint8_t *col = &canvasBuffer[page * ASTRA_SCREEN_W + x0];
    int span = ASTRA_SCREEN_W;  /* 页间距 */
    for (int y = y0; y <= y1; ) {
      uint8_t bit = 1 << bitIdx;
      if (drawType == 1)      *col |= bit;
      else if (drawType == 2) *col ^= bit;
      else                    *col &= ~bit;
      bitIdx++;
      y++;
      if (bitIdx == 8) { bitIdx = 0; col += span; }
    }
  }

  void _drawHDottedLine(float _x, float _y, float _l) override {
    for (int i = 0; i < (int)_l; i += 2) _drawPixel(_x + i, _y);
  }

  void _drawVDottedLine(float _x, float _y, float _h) override {
    for (int i = 0; i < (int)_h; i += 2) _drawPixel(_x, _y + i);
  }

  void _drawBox(float _x, float _y, float _w, float _h) override {
    int x0 = (int)_x, y0 = (int)_y + UI_OFFSET_Y, w = (int)_w, h = (int)_h;
    if (w <= 0 || h <= 0) return;
    int x1 = x0 + w - 1;
    int y1 = y0 + h - 1;
    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    if (x1 >= ASTRA_SCREEN_W) x1 = ASTRA_SCREEN_W - 1;
    if (y1 >= UI_MAX_Y) y1 = UI_MAX_Y - 1;
    if (x0 > x1 || y0 > y1) return;
    uint8_t op = drawType;
    for (int y = y0; y <= y1; y++) {
      uint8_t bit = 1 << (y % 8);
      uint8_t *row = &canvasBuffer[(y / 8) * ASTRA_SCREEN_W];
      if (op == 1)      { for (int x = x0; x <= x1; x++) row[x] |= bit; }
      else if (op == 2) { for (int x = x0; x <= x1; x++) row[x] ^= bit; }
      else              { uint8_t nb = ~bit; for (int x = x0; x <= x1; x++) row[x] &= nb; }
    }
  }

  void _drawFrame(float _x, float _y, float _w, float _h) override {
    _drawHLine(_x, _y, _w);
    _drawHLine(_x, _y + _h - 1, _w);
    _drawVLine(_x, _y, _h);
    _drawVLine(_x + _w - 1, _y, _h);
  }

  void _drawRBox(float _x, float _y, float _w, float _h, float _r) override {
    int x = (int)_x, y = (int)_y, w = (int)_w, h = (int)_h;
    int r = (int)_r;
    if (r <= 0) { _drawBox(_x, _y, _w, _h); return; }
    if (r > w / 2) r = w / 2;
    if (r > h / 2) r = h / 2;
    /* 中心十字填充 */
    _drawBox(x + r, y, w - 2 * r, h);
    _drawBox(x, y + r, w, h - 2 * r);
    /* 四角圆弧填充 */
    for (int dy = -r; dy <= r; dy++) {
      for (int dx = -r; dx <= r; dx++) {
        if (dx * dx + dy * dy <= r * r) {
          _drawPixel(x + r + dx, y + r + dy);
          _drawPixel(x + w - r - 1 + dx, y + r + dy);
          _drawPixel(x + r + dx, y + h - r - 1 + dy);
          _drawPixel(x + w - r - 1 + dx, y + h - r - 1 + dy);
        }
      }
    }
  }

  void _drawRFrame(float _x, float _y, float _w, float _h, float _r) override {
    int x = (int)_x, y = (int)_y, w = (int)_w, h = (int)_h;
    int r = (int)_r;
    if (r <= 0) { _drawFrame(_x, _y, _w, _h); return; }
    if (r > w / 2) r = w / 2;
    if (r > h / 2) r = h / 2;
    /* 直边 */
    _drawHLine(x + r, y, w - 2 * r);
    _drawHLine(x + r, y + h - 1, w - 2 * r);
    _drawVLine(x, y + r, h - 2 * r);
    _drawVLine(x + w - 1, y + r, h - 2 * r);
    /* 圆角弧线 */
    for (int dy = 0; dy <= r; dy++) {
      for (int dx = 0; dx <= r; dx++) {
        int d2 = dx * dx + dy * dy;
        if (d2 <= r * r && d2 >= (r - 1) * (r - 1)) {
          _drawPixel(x + r - dx, y + r - dy);
          _drawPixel(x + w - r - 1 + dx, y + r - dy);
          _drawPixel(x + r - dx, y + h - r - 1 + dy);
          _drawPixel(x + w - r - 1 + dx, y + h - r - 1 + dy);
        }
      }
    }
  }

  void _drawBMP(float _x, float _y, float _w, float _h, const uint8_t *_bitMap) override {
    if (!_bitMap) return;
    int w = (int)_w, h = (int)_h;
    /* 行优先 XBM 格式 (LSB first): byteIdx = y*((w+7)/8) + x/8, bit = 1<<(x%8) */
    int bytesPerRow = (w + 7) / 8;
    for (int y = 0; y < h; y++) {
      for (int x = 0; x < w; x++) {
        uint16_t byteIdx = y * bytesPerRow + (x / 8);
        uint8_t bit = 1 << (x % 8);
        if (_bitMap[byteIdx] & bit) _drawPixel(_x + x, _y + y);
      }
    }
  }

  /* ---- 文字绘制 ---- */
  /* _y 是左下角坐标, 转为左上角: yTop = _y - fontHeight */
  void _drawEnglish(float _x, float _y, const std::string &_text) override {
    float yTop = _y - 16;
    for (size_t i = 0; i < _text.length(); i++) {
      char c = _text[i];
      if (c < ' ' || c > '~') c = ' ';
      const unsigned char *glyph = font_8x16_data[c - ' '];
      for (int row = 0; row < 16; row++) {
        unsigned char b = glyph[row];
        for (int col = 0; col < 8; col++) {
          if (b & (0x80 >> col)) _drawPixel(_x + i * 8 + col, yTop + row);
        }
      }
    }
  }

  /* 无中文字库, 转发到英文绘制 (ASCII 部分正常, 非ASCII显示为空格) */
  void _drawChinese(float _x, float _y, const std::string &_text) override {
    _drawEnglish(_x, _y, _text);
  }

  /* ---- 定时 ---- */
  void _delay(unsigned long _mill) override { HAL_Delay(_mill); }
  unsigned long _millis() override { return HAL_GetTick(); }
  unsigned long _getTick() override { return HAL_GetTick(); }
  unsigned long _getRandomSeed() override {
    return *(uint32_t *)0x1FFFF7E8 ^ HAL_GetTick();
  }

  /* ---- 蜂鸣器 / 屏幕电源 (不使用) ---- */
  void _beep(float) override {}
  void _beepStop() override {}
  void _setBeepVol(uint8_t) override {}
  void _screenOn() override {}
  void _screenOff() override {}

  /* ---- 按键: 编码器旋钮 + SW 按钮 ---- */
  /* KEY_0 CLICK = 上一个 (编码器 CCW)
   * KEY_1 CLICK = 下一个 (编码器 CW)
   * KEY_1 PRESS = 打开/进入 (SW 短按 < 1s)
   * KEY_0 PRESS = 返回/关闭 (SW 长按 >= 1s)
   */
  bool _getKey(key::KEY_INDEX) override { return false; }  /* 由 _keyScan 直接处理 */

  bool _getAnyKey() override {
    /* 检查是否有待处理的按键动作 (非物理按键状态) */
    for (int i = 0; i < key::KEY_NUM; i++) {
      if (key[i] != key::RELEASE) return true;
    }
    return false;
  }

  void _keyScan() override {
    /* --- 编码器旋转 (4倍频, 累加到阈值才触发一次移动) --- */
    static int16_t encAccum = 0;
    int16_t encCount = Encoder_GetCount();
    if (encCount != 0) {
      encAccum += encCount;
      Encoder_ResetCount();
    }
    /* 阈值 4: 每 4 个计数 (一格) 触发一次 */
    if (encAccum >= 4) {
      key[key::KEY_1] = key::CLICK;   /* CW = 下一个 */
      key[key::KEY_0] = key::RELEASE;
      encAccum = 0;
      return;
    } else if (encAccum <= -4) {
      key[key::KEY_0] = key::CLICK;   /* CCW = 上一个 */
      key[key::KEY_1] = key::RELEASE;
      encAccum = 0;
      return;
    }

    /* --- 编码器 SW 按键 (短按=进入, 长按=返回) --- */
    uint8_t sw = Encoder_SW_Read();  /* 1=按下, 0=释放 */
    uint32_t now = HAL_GetTick();

    if (sw && !swPressed) {
      swPressed = true;
      swPressTime = now;
    }

    if (sw && swPressed && !swLongTriggered && (now - swPressTime >= 1000)) {
      key[key::KEY_0] = key::PRESS;   /* 长按 = 返回 */
      key[key::KEY_1] = key::RELEASE;
      swLongTriggered = true;
    }

    if (!sw && swPressed) {
      swPressed = false;
      if (!swLongTriggered && (now - swPressTime < 1000)) {
        key[key::KEY_1] = key::PRESS; /* 短按 = 进入 */
        key[key::KEY_0] = key::RELEASE;
      }
      swLongTriggered = false;
    }
  }

private:
  bool swPressed = false;
  uint32_t swPressTime = 0;
  bool swLongTriggered = false;
};

extern "C" int astraHalInit(void) {
  HAL *hal = new AstraHALPort();
  if (!HAL::inject(hal)) {
    delete hal;
    return -1;
  }
  return 0;
}
