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
#include "app_config.h"
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
#define STATUS_BAR_H   20   /* 顶部状态栏高度 (像素) */
#define BOTTOM_BAR_H   32   /* 底部状态栏高度 (像素, 顶部的 2 倍) */
#define UI_OFFSET_Y    STATUS_BAR_H   /* UI 内容在 canvasBuffer 中的 y 偏移 (避开顶部状态栏) */
#define UI_MAX_Y       (ASTRA_SCREEN_H - BOTTOM_BAR_H)  /* UI 内容最大 y 坐标 = 128, 避开底部状态栏 */
static char statusBarTitle[20] = {0};  /* 顶部状态栏标题文本 */

/* ---- 底部状态栏 FPS 计算 ---- */
static uint32_t fpsLastTick = 0;
static uint32_t fpsFrameCount = 0;
static uint8_t  currentFps = 0;

/* 开机动画标志 — 置 true 时跳过状态栏绘制 */
bool bootScreenActive = false;

static int getDrawOffsetY() {
  return bootScreenActive ? 0 : UI_OFFSET_Y;
}

static int getDrawMaxY() {
  return bootScreenActive ? ASTRA_SCREEN_H : UI_MAX_Y;
}

/* 设置状态栏标题 (供 launcher 调用) */
extern "C" void astraSetStatusBarTitle(const char *title) {
  if (title == nullptr) { statusBarTitle[0] = 0; return; }
  int i = 0;
  for (; i < (int)sizeof(statusBarTitle) - 1 && title[i]; i++) statusBarTitle[i] = title[i];
  statusBarTitle[i] = 0;
}

/* ---- 顶部状态栏右侧图标 ---- */
#define ICON_GAP 2
#define STATUS_TEXT_W 8
#define STATUS_TEXT_H 16

typedef struct StatusIcon {
  const uint8_t *data;
  uint8_t w;
  uint8_t h;
} StatusIcon;

/* 电池 16×10 (横向, 左侧正极凸起, 填充约 80%, 偏右 1px) */
static const uint8_t icon_battery_bitmap[] = {
  /* Row 0 */ 0x00, 0x00,
  /* Row 1: body top (14px, p2~p15) */ 0xFC, 0xFF,
  /* Row 2: leftmost column clipped */ 0x06, 0x80,
  /* Row 3: leftmost column clipped */ 0xF6, 0xBF,
  /* Row 4 */ 0xF6, 0xBF,
  /* Row 5 */ 0xF6, 0xBF,
  /* Row 6 */ 0xF6, 0xBF,
  /* Row 7: leftmost column clipped */ 0x06, 0x80,
  /* Row 8: body bottom */ 0xFC, 0xFF,
  /* Row 9 */ 0x00, 0x00,
};
static const StatusIcon STATUS_ICON_BATTERY = { icon_battery_bitmap, 16, 10 };

/*
 * WiFi 14x10, locked pixel art.
 * Geometry: two concentric arcs plus one center dot, shifted 1px left.
 * Keep the pixel art fixed unless the status icon is being tuned.
 *
 * Pixel preview (# = on):
 * ..............
 * ........####..
 * .......##.....
 * .....##.......
 * .....#...###..
 * ....#...##....
 * ...##..#......
 * ...#..##....#.
 * ...#..#...##..
 * ...#..#..###..
 */
static const uint8_t icon_wifi_bitmap[] = {
  /* Row 0 */ 0x00, 0x00,
  /* Row 1 */ 0x00, 0x0F,
  /* Row 2 */ 0x80, 0x01,
  /* Row 3 */ 0x60, 0x00,
  /* Row 4 */ 0x20, 0x0E,
  /* Row 5 */ 0x10, 0x03,
  /* Row 6 */ 0x98, 0x00,
  /* Row 7 */ 0xC8, 0x08,
  /* Row 8 */ 0x48, 0x0C,
  /* Row 9 */ 0x48, 0x0E,
};
static const StatusIcon STATUS_ICON_WIFI = { icon_wifi_bitmap, 14, 10 };

/* TF card 14x10, filled silhouette with clipped upper-right corner and contact slots */
static const uint8_t icon_tf_bitmap[] = {
  /* Row 0 */ 0x00, 0x00,
  /* Row 1 */ 0xFE, 0x03,
  /* Row 2 */ 0xFE, 0x0F,
  /* Row 3 */ 0xFE, 0x21,
  /* Row 4 */ 0xFE, 0x3F,
  /* Row 5 */ 0xFE, 0x21,
  /* Row 6 */ 0xFE, 0x3F,
  /* Row 7 */ 0xFE, 0x21,
  /* Row 8 */ 0xFE, 0x3F,
  /* Row 9 */ 0x00, 0x00,
};
static const StatusIcon STATUS_ICON_TF = { icon_tf_bitmap, 14, 10 };

static const StatusIcon *const STATUS_ICON_LAYOUT[] = {
  &STATUS_ICON_BATTERY,
  &STATUS_ICON_WIFI,
  &STATUS_ICON_TF,
};

/* 绘制小型图标到 canvasBuffer (LSB-first, 直接写显存, 支持多字节行宽) */
static void setCanvasPixelDirect(int x, int y) {
  if (x >= 0 && x < ASTRA_SCREEN_W && y >= 0 && y < ASTRA_SCREEN_H) {
    canvasBuffer[x + (y / 8) * ASTRA_SCREEN_W] |= (1 << (y % 8));
  }
}

static void drawIcon(int x, int y, const StatusIcon *icon) {
  int bytesPerRow = (icon->w + 7) / 8;
  for (int row = 0; row < icon->h; row++) {
    for (int col = 0; col < icon->w; col++) {
      int byteIdx = row * bytesPerRow + col / 8;
      int bitIdx  = col % 8;
      if (icon->data[byteIdx] & (1 << bitIdx)) {
        setCanvasPixelDirect(x + col, y + row);
      }
    }
  }
}

static int drawStatusIconFromRight(int rightX, const StatusIcon *icon) {
  int x = rightX - icon->w;
  int y = (STATUS_BAR_H - icon->h) / 2;
  drawIcon(x, y, icon);
  return x - ICON_GAP;
}

static int drawStatusIcons(void) {
  int iconRight = ASTRA_SCREEN_W - ICON_GAP;
  for (size_t i = 0; i < sizeof(STATUS_ICON_LAYOUT) / sizeof(STATUS_ICON_LAYOUT[0]); i++) {
    iconRight = drawStatusIconFromRight(iconRight, STATUS_ICON_LAYOUT[i]);
  }
  return iconRight - ICON_GAP;
}

static void drawStatusChar(int x, int y, char c) {
  if (c < ' ' || c > '~') c = ' ';
  const unsigned char *glyph = font_8x16_data[c - ' '];
  for (int row = 0; row < 16; row++) {
    unsigned char b = glyph[row];
    for (int col = 0; col < 8; col++) {
      if (b & (0x80 >> col)) setCanvasPixelDirect(x + col, y + row);
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

  /* 2. 右侧静态图标: TF卡 | WiFi | 电池 */
  int titleRightLimit = drawStatusIcons();

  /* 3. 画标题文字 (左侧, 保持 8x16 原始大小并垂直居中) */
  {
    int x = 0;
    int y = (STATUS_BAR_H - STATUS_TEXT_H) / 2;
    for (size_t i = 0; i < sizeof(statusBarTitle) && statusBarTitle[i]; i++) {
      if (x + STATUS_TEXT_W > titleRightLimit) break;
      drawStatusChar(x, y, statusBarTitle[i]);
      x += STATUS_TEXT_W;
    }
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
      if (b & (0x80 >> col)) setCanvasPixelDirect(x + col, y + row);
    }
  }
}

#if defined(__CC_ARM)
extern "C" unsigned int Image$$RW_IRAM1$$Base;
extern "C" unsigned int Image$$ER_IROM1$$Length;
extern "C" unsigned int Image$$RW_IRAM1$$Length;
extern "C" unsigned int __initial_sp;
#endif

static void appendChar(char *buf, int *pos, char c) {
  if (*pos < 16) buf[(*pos)++] = c;
}

static void appendText(char *buf, int *pos, const char *text) {
  for (int i = 0; text[i]; i++) appendChar(buf, pos, text[i]);
}

static void appendUInt(char *buf, int *pos, uint32_t value) {
  if (value >= 100U) appendChar(buf, pos, '0' + (value / 100U) % 10U);
  if (value >= 10U) appendChar(buf, pos, '0' + (value / 10U) % 10U);
  appendChar(buf, pos, '0' + value % 10U);
}

static uint32_t calcPercent(uint32_t usedBytes, uint32_t totalBytes) {
  if (totalBytes == 0U) return 0U;
  uint32_t percent = (usedBytes * 100U + totalBytes / 2U) / totalBytes;
  return (percent > 100U) ? 100U : percent;
}

extern "C" void astraGetMemoryUsage(AstraMemoryUsage *usage) {
  if (usage == nullptr) return;
#if defined(__CC_ARM)
  const uint32_t sramTotalBytes = 20U * 1024U;
  const uint32_t flashTotalBytes = 64U * 1024U;
  usage->ramBytes = (uint32_t)&__initial_sp - (uint32_t)&Image$$RW_IRAM1$$Base;
  usage->romBytes = (uint32_t)&Image$$ER_IROM1$$Length + (uint32_t)&Image$$RW_IRAM1$$Length;
#else
  const uint32_t sramTotalBytes = 1U;
  const uint32_t flashTotalBytes = 1U;
  usage->ramBytes = 0;
  usage->romBytes = 0;
#endif
  usage->ramPercent = calcPercent(usage->ramBytes, sramTotalBytes);
  usage->romPercent = calcPercent(usage->romBytes, flashTotalBytes);
}

static void appendPercentMetric(char *buf, int *pos, const char *label, uint32_t percent) {
  appendText(buf, pos, label);
  appendUInt(buf, pos, percent);
  appendChar(buf, pos, '%');
}

static void drawStringDirect(int x, int y, const char *text) {
  for (int i = 0; text[i] && x + 8 <= ASTRA_SCREEN_W; i++) {
    drawCharDirect(x, y, text[i]);
    x += 8;
  }
}

static void buildMemoryInfoText(char *buf) {
  int pos = 0;
  AstraMemoryUsage usage;
  astraGetMemoryUsage(&usage);
  appendPercentMetric(buf, &pos, "RAM:", usage.ramPercent);
  appendChar(buf, &pos, ' ');
  appendPercentMetric(buf, &pos, "ROM:", usage.romPercent);
  buf[pos] = 0;
}

static void buildTimeText(char *buf) {
  uint32_t seconds = HAL_GetTick() / 1000U;
  uint32_t hours = (seconds / 3600U) % 100U;
  uint32_t minutes = (seconds / 60U) % 60U;
  seconds %= 60U;

  buf[0] = '0' + hours / 10U;
  buf[1] = '0' + hours % 10U;
  buf[2] = ':';
  buf[3] = '0' + minutes / 10U;
  buf[4] = '0' + minutes % 10U;
  buf[5] = ':';
  buf[6] = '0' + seconds / 10U;
  buf[7] = '0' + seconds % 10U;
  buf[8] = 0;
}

static void buildFpsText(char *buf) {
  buf[0] = 'F'; buf[1] = 'P'; buf[2] = 'S'; buf[3] = ':';
  uint8_t fps = currentFps;
  if (fps == 0) {
    buf[4] = '0';
    buf[5] = 0;
  } else {
    buf[5] = '0' + fps % 10;
    fps /= 10;
    buf[4] = (fps > 0) ? ('0' + fps) : ' ';
    buf[6] = 0;
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

  /* 3. 第一行文字: RAM/ROM 占用 */
  {
    char memBuf[17];
    buildMemoryInfoText(memBuf);
    drawStringDirect(0, UI_MAX_Y + 1, memBuf);
  }

  /* 4. 第二行文字: 左侧时间, 右侧 FPS */
  {
    char timeBuf[9];
    char fpsBuf[8];
    buildTimeText(timeBuf);
    buildFpsText(fpsBuf);
    int fpsLen = 0;
    while (fpsBuf[fpsLen]) fpsLen++;
    drawStringDirect(0, UI_MAX_Y + 17, timeBuf);
    drawStringDirect(ASTRA_SCREEN_W - fpsLen * 8, UI_MAX_Y + 17, fpsBuf);
  }
}

#if defined(__CC_ARM)
#pragma diag_suppress 1300
#endif

class AstraHALPort : public HAL {
public:
  std::string type() { return "STM32F103_ST7735S"; }

  /* ---- 画布缓冲 ---- */
  void *_getCanvasBuffer() { return canvasBuffer; }
  uint8_t _getBufferTileHeight() { return ASTRA_SCREEN_H / 8; }  /* 20 */
  uint8_t _getBufferTileWidth() { return ASTRA_SCREEN_W; }       /* 128 */

  /* ---- 画布刷新: 1bpp → RGB565 → LCD (流式写入, 单次窗口设置) ---- */
  void _canvasUpdate() {
    if (!bootScreenActive) {
      drawStatusBar();
      drawBottomStatusBar();
    }

    /* FPS 计算 (基于 canvasUpdate 调用次数) */
    if (!bootScreenActive) {
      fpsFrameCount++;
      uint32_t now = HAL_GetTick();
      if (now - fpsLastTick >= 1000) {
        currentFps = (uint8_t)(fpsFrameCount * 1000 / (now - fpsLastTick));
        fpsFrameCount = 0;
        fpsLastTick = now;
      }
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

  void _canvasClear() {
    memset(canvasBuffer, 0, ASTRA_BUF_SIZE);
  }

  /* ---- 字体 ---- */
  void _setFont(const uint8_t *) {}  /* 使用内置 8x16, 无需外部字体 */

  uint8_t _getFontWidth(std::string &_text) {
    return (uint8_t)(_text.length() * 8);
  }

  uint8_t _getFontHeight() { return 16; }

  /* ---- 绘图原语 ---- */
  uint8_t drawType = 1;

  void _setDrawType(uint8_t _type) { drawType = _type; }

  void _drawPixel(float _x, float _y) {
    int x = (int)_x, y = (int)_y + getDrawOffsetY();
    if (x < 0 || x >= ASTRA_SCREEN_W || y < 0 || y >= getDrawMaxY()) return;
    uint16_t idx = x + (y / 8) * ASTRA_SCREEN_W;
    uint8_t bit = 1 << (y % 8);
    if (drawType == 1) canvasBuffer[idx] |= bit;
    else if (drawType == 2) canvasBuffer[idx] ^= bit;
    else canvasBuffer[idx] &= ~bit;
  }

  /* 优化: 直接字节操作, 避免 float→int 和边界检查开销 */
  void _drawHLine(float _x, float _y, float _l) {
    int x0 = (int)_x, y0 = (int)_y + getDrawOffsetY(), l = (int)_l;
    int maxY = getDrawMaxY();
    if (l <= 0 || y0 < 0 || y0 >= maxY) return;
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

  void _drawVLine(float _x, float _y, float _h) {
    int x0 = (int)_x, y0 = (int)_y + getDrawOffsetY(), h = (int)_h;
    int maxY = getDrawMaxY();
    if (h <= 0 || x0 < 0 || x0 >= ASTRA_SCREEN_W) return;
    int y1 = y0 + h - 1;
    if (y0 < 0) y0 = 0;
    if (y1 >= maxY) y1 = maxY - 1;
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

  void _drawHDottedLine(float _x, float _y, float _l) {
    for (int i = 0; i < (int)_l; i += 2) _drawPixel(_x + i, _y);
  }

  void _drawVDottedLine(float _x, float _y, float _h) {
    for (int i = 0; i < (int)_h; i += 2) _drawPixel(_x, _y + i);
  }

  void _drawBox(float _x, float _y, float _w, float _h) {
    int x0 = (int)_x, y0 = (int)_y + getDrawOffsetY(), w = (int)_w, h = (int)_h;
    int maxY = getDrawMaxY();
    if (w <= 0 || h <= 0) return;
    int x1 = x0 + w - 1;
    int y1 = y0 + h - 1;
    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    if (x1 >= ASTRA_SCREEN_W) x1 = ASTRA_SCREEN_W - 1;
    if (y1 >= maxY) y1 = maxY - 1;
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

  void _drawFrame(float _x, float _y, float _w, float _h) {
    _drawHLine(_x, _y, _w);
    _drawHLine(_x, _y + _h - 1, _w);
    _drawVLine(_x, _y, _h);
    _drawVLine(_x + _w - 1, _y, _h);
  }

  void _drawRBox(float _x, float _y, float _w, float _h, float _r) {
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

  void _drawRFrame(float _x, float _y, float _w, float _h, float _r) {
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

  void _drawBMP(float _x, float _y, float _w, float _h, const uint8_t *_bitMap) {
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
  void _drawEnglish(float _x, float _y, const std::string &_text) {
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
  void _drawChinese(float _x, float _y, const std::string &_text) {
    _drawEnglish(_x, _y, _text);
  }

  /* ---- 定时 ---- */
  void _delay(unsigned long _mill) { HAL_Delay(_mill); }
  unsigned long _millis() { return HAL_GetTick(); }
  unsigned long _getTick() { return HAL_GetTick(); }
  unsigned long _getRandomSeed() {
    return *(uint32_t *)0x1FFFF7E8 ^ HAL_GetTick();
  }

  /* ---- 蜂鸣器 / 屏幕电源 (不使用) ---- */
  void _beep(float) {}
  void _beepStop() {}
  void _setBeepVol(uint8_t) {}
  void _screenOn() {}
  void _screenOff() {}

  /* ---- 按键: 编码器旋钮 + SW 按钮 ---- */
  /* KEY_0 CLICK = 上一个 (编码器 CCW)
   * KEY_1 CLICK = 下一个 (编码器 CW)
   * KEY_1 PRESS = 打开/进入 (SW 短按 < 1s)
   * KEY_0 PRESS = 返回/关闭 (SW 长按 >= 1s)
   */
  bool _getKey(key::KEY_INDEX) { return false; }  /* 由 _keyScan 直接处理 */

  bool _getAnyKey() {
    /* 检查是否有待处理的按键动作 (非物理按键状态) */
    for (int i = 0; i < key::KEY_NUM; i++) {
      if (key[i] != key::RELEASE) return true;
    }
    return false;
  }

  void _keyScan() {
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

#if defined(__CC_ARM)
#pragma diag_default 1300
#endif

extern "C" int astraHalInit(void) {
  HAL *hal = new AstraHALPort();
  if (!HAL::inject(hal)) {
    delete hal;
    return -1;
  }
  return 0;
}

