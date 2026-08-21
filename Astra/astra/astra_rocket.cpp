//
// Created by Fir on 2024/3/7 007.
// 移植: STM32F103C8T6 + ST7735S 128x160 + 编码器旋钮
// 改造:
//   - 菜单项改为 ASCII (字体仅支持 0x20~0x7E)
//   - 新增 astraLoop() 非阻塞接口, 供 main 主循环调用
//

#include "astra_rocket.h"
#include "astra_icons.h"
#include "ui/launcher.h"
#include "app_config.h"
#include "app_log.h"
#include "bmp280.h"
#include "dht11.h"
#include "i2c.h"

/* hal_port.cpp 中定义, 控制状态栏绘制 */
extern bool bootScreenActive;

static astra::Launcher* astraLauncher = nullptr;
static astra::Menu* rootPage = nullptr;
static astra::Menu* toolPage = nullptr;
static uint32_t dht11PageLastReadTick = 0;
static uint8_t dht11PageHasAttemptedRead = 0;
static BMP280_Device bmp280Device;
static BMP280_Reading bmp280PageReading = {0, 0U, BMP280_ERR_NOT_INITIALIZED};
static BMP280_Status bmp280PageInitStatus = BMP280_ERR_NOT_INITIALIZED;
static uint32_t bmp280PageLastReadTick = 0;
static uint8_t bmp280PageHasAttemptedRead = 0;
static uint8_t bmp280PageBusInitialized = 0;

namespace astra {
config &getUIConfig() {
  static config astraConfig;
  return astraConfig;
}
}

static size_t asciiLength(const char *text) {
  size_t len = 0;
  while (text != nullptr && text[len]) len++;
  return len;
}

static void drawCenteredText(const char *text, float baselineY) {
  HAL::drawEnglish((128.0f - (float)(asciiLength(text) * 8)) / 2.0f, baselineY, text);
}

static astra::Menu* createTile(const char *title, const uint8_t *icon, uint16_t iconSize) {
  return new astra::Menu(title, icon, iconSize);
}

static void addLeaf(astra::Menu *page, const char *title) {
  page->addItem(new astra::Menu(title));
}

static void drawUInt(uint8_t x, uint8_t y, uint32_t value) {
  char buf[11];
  int len = 0;
  if (value == 0) {
    buf[len++] = '0';
  } else {
    char tmp[10];
    int tmpLen = 0;
    while (value > 0 && tmpLen < 10) {
      tmp[tmpLen++] = (char)('0' + value % 10U);
      value /= 10U;
    }
    while (tmpLen > 0) buf[len++] = tmp[--tmpLen];
  }
  buf[len] = 0;
  HAL::drawEnglish(x, y, buf);
}

static const char *dht11PageStatusText(DHT11_Status status);

static void appendLogText(char *buf, int *pos, const char *text) {
  for (int i = 0; text[i]; i++) buf[(*pos)++] = text[i];
}

static void appendLogUInt(char *buf, int *pos, uint32_t value) {
  if (value == 0) {
    buf[(*pos)++] = '0';
    return;
  }

  char tmp[10];
  int len = 0;
  while (value > 0 && len < 10) {
    tmp[len++] = (char)('0' + value % 10U);
    value /= 10U;
  }
  while (len > 0) buf[(*pos)++] = tmp[--len];
}

static void appendFixed2(char *buf, int *pos, int32_t value) {
  uint32_t magnitude;
  if (value < 0) {
    buf[(*pos)++] = '-';
    magnitude = (uint32_t)(-(int64_t)value);
  } else {
    magnitude = (uint32_t)value;
  }

  appendLogUInt(buf, pos, magnitude / 100U);
  buf[(*pos)++] = '.';
  buf[(*pos)++] = (char)('0' + (magnitude / 10U) % 10U);
  buf[(*pos)++] = (char)('0' + magnitude % 10U);
}

static void appendHexByte(char *buf, int *pos, uint8_t value) {
  static const char hex[] = "0123456789ABCDEF";
  buf[(*pos)++] = '0';
  buf[(*pos)++] = 'x';
  buf[(*pos)++] = hex[value >> 4];
  buf[(*pos)++] = hex[value & 0x0FU];
}

static void drawMeasurementCentered(const char *prefix, int32_t value,
                                    const char *suffix, uint8_t baselineY) {
  char buf[32];
  int p = 0;
  appendLogText(buf, &p, prefix);
  appendFixed2(buf, &p, value);
  appendLogText(buf, &p, suffix);
  buf[p] = 0;
  drawCenteredText(buf, baselineY);
}

static void logDht11Reading(const DHT11_Reading *reading) {
  char buf[128];
  int p = 0;
  appendLogText(buf, &p, "[INFO] [dht11] status=");
  appendLogText(buf, &p, dht11PageStatusText(reading->status));
  appendLogText(buf, &p, " raw=");
  appendLogText(buf, &p, DHT11_StatusText(reading->status));
  if (reading->status == DHT11_OK) {
    appendLogText(buf, &p, " temp=");
    appendLogUInt(buf, &p, reading->temperature);
    appendLogText(buf, &p, " humi=");
    appendLogUInt(buf, &p, reading->humidity);
  }
  appendLogText(buf, &p, "\r\n");
  buf[p] = 0;
  uart_puts(buf);
}

static const char *dht11PageStatusText(DHT11_Status status) {
  switch (status) {
    case DHT11_OK:
      return "OK";
    case DHT11_ERR_RESPONSE_HIGH:
    case DHT11_ERR_RESPONSE_LOW:
    case DHT11_ERR_RESPONSE_RELEASE:
      return "No Sensor";
    case DHT11_ERR_CHECKSUM:
      return "Data error";
    case DHT11_ERR_BIT_LOW:
    case DHT11_ERR_BIT_HIGH:
    default:
      return "Sensor error";
  }
}

static void dht11PageEnter(void) {
  dht11PageLastReadTick = 0;
  dht11PageHasAttemptedRead = 0;
  DHT11_Init();
  APP_LOG_INFO("dht11", "page_enter init=1 pin=PA12");
}

static void dht11PageExit(void) {
  DHT11_DeInit();
  dht11PageLastReadTick = 0;
  dht11PageHasAttemptedRead = 0;
  APP_LOG_INFO("dht11", "page_exit released=1 pin=PA12");
}

static void drawDht11Page(void) {
  uint32_t now = HAL_GetTick();
  if (!dht11PageHasAttemptedRead || now - dht11PageLastReadTick >= 2000U) {
    DHT11_Reading reading = DHT11_Read();
    logDht11Reading(&reading);
    dht11PageLastReadTick = now;
    dht11PageHasAttemptedRead = 1;
  }

  DHT11_Reading reading;
  uint8_t hasReading = DHT11_GetLastReading(&reading);

  HAL::drawEnglish(28, 34, "Temp/Humi");
  HAL::drawHLine(10, 42, 108);

  if (!hasReading || reading.status != DHT11_OK) {
    HAL::drawEnglish(18, 78, hasReading ? dht11PageStatusText(reading.status) : "No Sensor");
    return;
  }

  HAL::drawEnglish(18, 72, "Temp:");
  drawUInt(66, 72, reading.temperature);
  HAL::drawEnglish(90, 72, "C");

  HAL::drawEnglish(18, 98, "Humi:");
  drawUInt(66, 98, reading.humidity);
  HAL::drawEnglish(90, 98, "%");
}

static bool bmp280BusIsReady(void *, uint8_t address7, uint32_t timeoutMs) {
  return I2C2_BusIsReady(address7, timeoutMs);
}

static bool bmp280BusRead(void *, uint8_t address7, uint8_t reg,
                          uint8_t *data, uint16_t size, uint32_t timeoutMs) {
  return I2C2_BusMemRead(address7, reg, data, size, timeoutMs);
}

static bool bmp280BusWrite(void *, uint8_t address7, uint8_t reg,
                           const uint8_t *data, uint16_t size,
                           uint32_t timeoutMs) {
  return I2C2_BusMemWrite(address7, reg, data, size, timeoutMs);
}

static void bmp280BusDelay(void *, uint32_t delayMs) {
  HAL_Delay(delayMs);
}

static const char *bmp280PageStatusText(BMP280_Status status) {
  switch (status) {
    case BMP280_OK:
      return "OK";
    case BMP280_ERR_BUS:
    case BMP280_ERR_NO_SENSOR:
    case BMP280_ERR_NOT_INITIALIZED:
      return "No Sensor";
    case BMP280_ERR_DATA:
    case BMP280_ERR_COMPENSATION:
    case BMP280_ERR_INVALID_ARGUMENT:
    case BMP280_ERR_CHIP_ID:
    case BMP280_ERR_CALIBRATION:
    case BMP280_ERR_CONFIG:
    default:
      return "Data error";
  }
}

static void logBmp280Connection(const char *event) {
  char buf[160];
  int p = 0;
  appendLogText(buf, &p, "[INFO] [bmp280] ");
  appendLogText(buf, &p, event);
  appendLogText(buf, &p, " status=");
  appendLogText(buf, &p, BMP280_StatusText(bmp280PageInitStatus));
  appendLogText(buf, &p, " bus=I2C2 scl=PB10 sda=PB11");
  if (bmp280PageInitStatus == BMP280_OK) {
    appendLogText(buf, &p, " address=");
    appendHexByte(buf, &p, BMP280_GetAddress(&bmp280Device));
    appendLogText(buf, &p, " id=");
    appendHexByte(buf, &p, BMP280_GetChipId(&bmp280Device));
  }
  appendLogText(buf, &p, "\r\n");
  buf[p] = 0;
  uart_puts(buf);
}

static void logBmp280Reading(const BMP280_Reading *reading) {
  char buf[160];
  int p = 0;
  appendLogText(buf, &p, "[INFO] [bmp280] status=");
  appendLogText(buf, &p, BMP280_StatusText(reading->status));
  if (reading->status == BMP280_OK) {
    appendLogText(buf, &p, " temp_c=");
    appendFixed2(buf, &p, reading->temperature_centi_c);
    appendLogText(buf, &p, " pressure_pa=");
    appendLogUInt(buf, &p, reading->pressure_pa);
    appendLogText(buf, &p, " pressure_hpa=");
    appendFixed2(buf, &p, (int32_t)reading->pressure_pa);
  }
  appendLogText(buf, &p, "\r\n");
  buf[p] = 0;
  uart_puts(buf);
}

static const BMP280_Bus bmp280Bus = {
  nullptr,
  bmp280BusIsReady,
  bmp280BusRead,
  bmp280BusWrite,
  bmp280BusDelay
};

static void bmp280PageRelease(void) {
  if (bmp280PageBusInitialized != 0U) {
    BMP280_DeInit(&bmp280Device);
    I2C2_BusDeInit();
  }
  bmp280PageBusInitialized = 0;
}

static bool bmp280PageConnect(const char *event) {
  bmp280PageRelease();
  bmp280PageReading.status = BMP280_ERR_NOT_INITIALIZED;
  bmp280PageHasAttemptedRead = 0;

  bmp280PageBusInitialized = I2C2_BusInit() ? 1U : 0U;
  if (bmp280PageBusInitialized == 0U) {
    bmp280PageInitStatus = BMP280_ERR_BUS;
  } else {
    bmp280PageInitStatus = BMP280_Init(&bmp280Device, &bmp280Bus);
    if (bmp280PageInitStatus != BMP280_OK) bmp280PageRelease();
  }

  logBmp280Connection(event);
  return bmp280PageInitStatus == BMP280_OK;
}

static void bmp280PageEnter(void) {
  bmp280PageLastReadTick = HAL_GetTick();
  bmp280PageConnect("page_enter");
}

static void bmp280PageExit(void) {
  bmp280PageRelease();
  bmp280PageInitStatus = BMP280_ERR_NOT_INITIALIZED;
  bmp280PageLastReadTick = 0;
  bmp280PageHasAttemptedRead = 0;
  bmp280PageReading.status = BMP280_ERR_NOT_INITIALIZED;
  APP_LOG_INFO("bmp280", "page_exit released=1 bus=I2C2");
}

static void drawBmp280Page(void) {
  HAL::drawEnglish(28, 34, "Barometer");
  HAL::drawHLine(10, 42, 108);

  uint32_t now = HAL_GetTick();
  if (bmp280PageInitStatus != BMP280_OK) {
    if (now - bmp280PageLastReadTick >= 2000U) {
      bmp280PageLastReadTick = now;
      bmp280PageConnect("reconnect");
    }
    if (bmp280PageInitStatus != BMP280_OK) {
      drawCenteredText(bmp280PageStatusText(bmp280PageInitStatus), 78);
      return;
    }
  }

  if (!bmp280PageHasAttemptedRead ||
      now - bmp280PageLastReadTick >= 2000U) {
    bmp280PageReading = BMP280_Read(&bmp280Device);
    logBmp280Reading(&bmp280PageReading);
    bmp280PageLastReadTick = now;
    bmp280PageHasAttemptedRead = 1;
    if (bmp280PageReading.status != BMP280_OK) {
      bmp280PageInitStatus = bmp280PageReading.status;
      bmp280PageRelease();
    }
  }

  if (bmp280PageReading.status != BMP280_OK) {
    drawCenteredText(bmp280PageStatusText(bmp280PageReading.status), 78);
    return;
  }

  drawCenteredText("Pressure", 60);
  drawMeasurementCentered("", (int32_t)bmp280PageReading.pressure_pa,
                          " hPa", 80);
  drawMeasurementCentered("Temp ", bmp280PageReading.temperature_centi_c,
                          " C", 102);
}

static void buildMenuTree(void) {
  if (rootPage != nullptr) return;

  rootPage = new astra::Menu("root");

  astra::Menu* homeTile = createTile("Home", pic_home_data, sizeof(pic_home_data));
  astra::Menu* sensorsTile = createTile("Sensors", pic_sensor_data, sizeof(pic_sensor_data));
  astra::Menu* aboutTile = createTile("About", pic_info_data, sizeof(pic_info_data));
  toolPage = createTile("Settings", pic_tool_data, sizeof(pic_tool_data));

  rootPage->addItem(homeTile);
  rootPage->addItem(sensorsTile);
  rootPage->addItem(aboutTile);
  rootPage->addItem(toolPage);

  addLeaf(homeTile, "-Status");
  addLeaf(homeTile, "-Uptime");
  addLeaf(homeTile, "-Memory");

  sensorsTile->addItem(new astra::Menu("-Temp/Humi", drawDht11Page, dht11PageEnter, dht11PageExit));
  sensorsTile->addItem(new astra::Menu("-Barometer", drawBmp280Page,
                                      bmp280PageEnter, bmp280PageExit));
  addLeaf(sensorsTile, "-Light");

  addLeaf(aboutTile, "-Astra UI");
  addLeaf(aboutTile, "-STM32F103");
  addLeaf(aboutTile, "-ST7735S");

  addLeaf(toolPage, "-Brightness");
  addLeaf(toolPage, "-Contrast");
  addLeaf(toolPage, "-Reset");
}

void astraShowBootScreen(void) {
  if (!HAL::check()) return;

  bootScreenActive = true;

  uint32_t startTime = HAL_GetTick();
  while (HAL_GetTick() - startTime < 2000) {
    HAL::canvasClear();
    HAL::setDrawType(1);

    drawCenteredText(APP_NAME, 48);
    drawCenteredText(APP_VERSION, 70);
    drawCenteredText(APP_TARGET_NAME, 92);
    drawCenteredText(APP_LCD_BOOT_NAME, 114);

    HAL::canvasUpdate();
  }

  bootScreenActive = false;
}

void astraCoreInit(void) {
  /* HAL 实例需由用户在调用本函数前注入 (在 main 中调用 astraHalInit) */
  if (!HAL::check()) return;
  APP_LOG_INFO("astra", "step=hal_check status=ok");

  astraShowBootScreen();

  /* 延迟分配全局对象 (避免 C++ 全局构造阶段崩溃) */
  if (astraLauncher == nullptr) astraLauncher = new astra::Launcher();
  APP_LOG_INFO("astra", "step=new_launcher status=ok");

  buildMenuTree();
  APP_LOG_INFO("astra", "step=build_menu_tree status=ok root_items=4");

  astraLauncher->init(rootPage);
  APP_LOG_INFO("astra", "step=launcher_init status=ok");
}

void astraCoreStart(void) {
  for (;;) {  //NOLINT
    astraLauncher->update();
  }
}

/* 非阻塞单次更新, 供 main 主循环调用 */
void astraLoop(void) {
  astraLauncher->update();
}

void astraCoreTest(void) {
  HAL::canvasClear();
  HAL::setDrawType(1);
  HAL::drawEnglish(0, 16, "Astra UI");
  HAL::drawEnglish(0, 32, "STM32F103");
  HAL::drawHLine(2, 0, 120);
  HAL::drawVLine(0, 0, 100);
  HAL::canvasUpdate();
}

void astraCoreDestroy(void) {
  HAL::destroy();
  delete astraLauncher;
  astraLauncher = nullptr;
  delete rootPage;
  rootPage = nullptr;
  toolPage = nullptr;
}
