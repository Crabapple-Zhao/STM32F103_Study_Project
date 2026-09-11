//
// Created by Fir on 2024/3/7 007.
// Port: STM32F103C8T6 + ST7735S 128x160 + rotary encoder.
//

#include "astra_rocket.h"
#include "astra_icons.h"
#include "pages/sensor_pages.h"
#include "pages/pwm_page.h"
#include "ui/launcher.h"
#include "app_config.h"
#include "app_log.h"

/* Defined in hal_port.cpp and used to suppress status bars during boot. */
extern bool bootScreenActive;

static astra::Launcher* astraLauncher = nullptr;
static astra::Menu* rootPage = nullptr;
static astra::Menu* toolPage = nullptr;

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
  HAL::drawEnglish((128.0f - (float)(asciiLength(text) * 8)) / 2.0f,
                   baselineY, text);
}

/* root_items 随实际磁贴数量输出, 增删入口无需同步修改日志 */
static void logBuildMenuTree(uint8_t rootItems) {
  char buf[4];
  int p = 0;
  if (rootItems == 0) {
    buf[p++] = '0';
  } else {
    char tmp[3];
    int n = 0;
    while (rootItems > 0) {
      tmp[n++] = (char)('0' + rootItems % 10);
      rootItems /= 10;
    }
    while (n > 0) buf[p++] = tmp[--n];
  }
  buf[p] = 0;
  uart_puts("[INFO] [astra] step=build_menu_tree status=ok root_items=");
  uart_puts(buf);
  uart_puts("\r\n");
}

static astra::Menu* createTile(const char *title, const astra::Icon *icon) {
  //资源校验: 尺寸与字节数不匹配时启动日志给出一次性提示, 渲染层跳过绘制
  if (icon != nullptr && icon->normal != nullptr) {
    bool badNormal = !icon->normal->valid();
    bool badSelected = icon->selected != nullptr && !icon->selected->valid();
    if (badNormal || badSelected) {
      uart_puts("[ERROR] [astra] icon resource invalid: ");
      uart_puts(title);
      uart_puts("\r\n");
    }
  }
  return new astra::Menu(title, icon);
}

static void addLeaf(astra::Menu *page, const char *title) {
  page->addItem(new astra::Menu(title));
}

static void buildMenuTree(void) {
  if (rootPage != nullptr) return;

  rootPage = new astra::Menu("root");

  astra::Menu* homeTile = createTile("Home", &icon_home);
  astra::Menu* sensorsTile = createTile("Sensors", &icon_sensors);
  astra::Menu* outputTile = createTile("Output", &icon_output);
  toolPage = createTile("Settings", &icon_settings);

  rootPage->addItem(homeTile);
  rootPage->addItem(sensorsTile);
  rootPage->addItem(outputTile);
  rootPage->addItem(toolPage);

  addLeaf(homeTile, "-Status");
  addLeaf(homeTile, "-Uptime");
  addLeaf(homeTile, "-Memory");

  SensorPages_AddToMenu(sensorsTile);

  PwmPage_AddToMenu(outputTile);

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
  /* HAL must be injected by astraHalInit() before this function. */
  if (!HAL::check()) return;
  APP_LOG_INFO("astra", "step=hal_check status=ok");

  astraShowBootScreen();

  /* Delay allocation until runtime to avoid global constructor failures. */
  if (astraLauncher == nullptr) astraLauncher = new astra::Launcher();
  APP_LOG_INFO("astra", "step=new_launcher status=ok");

  buildMenuTree();
  logBuildMenuTree(rootPage->getItemNum());

  astraLauncher->init(rootPage);
  APP_LOG_INFO("astra", "step=launcher_init status=ok");
}

void astraCoreStart(void) {
  for (;;) {  // NOLINT
    astraLauncher->update();
  }
}

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
