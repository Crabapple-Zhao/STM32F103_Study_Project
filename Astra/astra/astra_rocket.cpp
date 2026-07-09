//
// Created by Fir on 2024/3/7 007.
// 移植: STM32F103C8T6 + ST7735S 128x160 + 编码器旋钮
// 改造:
//   - 菜单项改为 ASCII (字体仅支持 0x20~0x7E)
//   - 新增 astraLoop() 非阻塞接口, 供 main 主循环调用
//

#include <vector>
#include "astra_rocket.h"
#include "astra_icons.h"
#include "ui/launcher.h"
#include "app_config.h"
#include "app_log.h"

/* hal_port.cpp 中定义, 控制状态栏绘制 */
extern bool bootScreenActive;

static astra::Launcher* astraLauncher = nullptr;
static astra::Menu* rootPage = nullptr;
static astra::Menu* toolPage = nullptr;

static std::vector<uint8_t> pic_home;
static std::vector<uint8_t> pic_gear;
static std::vector<uint8_t> pic_info;
static std::vector<uint8_t> pic_tool;

static void drawCenteredText(const std::string &text, float baselineY) {
  HAL::drawEnglish((128.0f - (float)(text.length() * 8)) / 2.0f, baselineY, text);
}

static void loadMenuIcons(void) {
  pic_home.assign(pic_home_data, pic_home_data + sizeof(pic_home_data));
  pic_gear.assign(pic_gear_data, pic_gear_data + sizeof(pic_gear_data));
  pic_info.assign(pic_info_data, pic_info_data + sizeof(pic_info_data));
  pic_tool.assign(pic_tool_data, pic_tool_data + sizeof(pic_tool_data));
}

static astra::Menu* createTile(const char *title, const std::vector<uint8_t> &icon) {
  return new astra::Menu(title, icon);
}

static void addLeaf(astra::Menu *page, const char *title) {
  page->addItem(new astra::Menu(title));
}

static void buildMenuTree(void) {
  if (rootPage != nullptr) return;

  rootPage = new astra::Menu("root");
  if (pic_home.empty()) loadMenuIcons();

  astra::Menu* homeTile = createTile("Home", pic_home);
  astra::Menu* settingsTile = createTile("Settings", pic_gear);
  astra::Menu* aboutTile = createTile("About", pic_info);
  toolPage = createTile("Tools", pic_tool);

  rootPage->addItem(homeTile);
  rootPage->addItem(settingsTile);
  rootPage->addItem(aboutTile);
  rootPage->addItem(toolPage);

  addLeaf(homeTile, "-Status");
  addLeaf(homeTile, "-Uptime");
  addLeaf(homeTile, "-Memory");

  addLeaf(settingsTile, "-Brightness");
  addLeaf(settingsTile, "-Contrast");
  addLeaf(settingsTile, "-Reset");

  addLeaf(aboutTile, "-Astra UI");
  addLeaf(aboutTile, "-STM32F103");
  addLeaf(aboutTile, "-ST7735S");

  addLeaf(toolPage, "-Encoder");
  addLeaf(toolPage, "-LCD Test");
  addLeaf(toolPage, "-LED Blink");
  addLeaf(toolPage, "-SPI DMA");
  addLeaf(toolPage, "-Key Scan");
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
