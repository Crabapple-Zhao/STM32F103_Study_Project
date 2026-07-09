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

extern "C" {
#include "usart.h"
}

/* hal_port.cpp 中定义, 控制状态栏绘制 */
extern bool bootScreenActive;

astra::Launcher* astraLauncher = nullptr;
astra::Menu* rootPage = nullptr;

std::vector<uint8_t> pic_home;  /* 延迟初始化 */
std::vector<uint8_t> pic_gear;  /* 延迟初始化 */
std::vector<uint8_t> pic_info;  /* 延迟初始化 */
std::vector<uint8_t> pic_tool;  /* 延迟初始化 */

/* 二级菜单 (工具页) */
astra::Menu* toolPage = nullptr;

/* 诊断辅助: 单次输出 */
static void dbgStep(const char* tag) {
  uart_puts(tag);
}

void astraShowBootScreen(void) {
  if (!HAL::check()) return;

  bootScreenActive = true;

  uint32_t startTime = HAL_GetTick();
  while (HAL_GetTick() - startTime < 2000) {
    HAL::canvasClear();
    HAL::setDrawType(1);

    std::string title = "Dev-Beta";
    HAL::drawEnglish((128.0f - (float)(title.length() * 8)) / 2.0f, 48, title);

    std::string ver = "v0.3.5";
    HAL::drawEnglish((128.0f - (float)(ver.length() * 8)) / 2.0f, 70, ver);

    std::string hw = "STM32F103C8T6";
    HAL::drawEnglish((128.0f - (float)(hw.length() * 8)) / 2.0f, 92, hw);

    std::string lcd = "ST7735S 128x160";
    HAL::drawEnglish((128.0f - (float)(lcd.length() * 8)) / 2.0f, 114, lcd);

    HAL::canvasUpdate();
  }

  bootScreenActive = false;
}

void astraCoreInit(void) {
  /* HAL 实例需由用户在调用本函数前注入 (在 main 中调用 astraHalInit) */
  if (!HAL::check()) return;
  dbgStep("[a1] check ok\r\n");

  astraShowBootScreen();

  /* 延迟分配全局对象 (避免 C++ 全局构造阶段崩溃) */
  if (astraLauncher == nullptr) astraLauncher = new astra::Launcher();
  dbgStep("[a2] new Launcher ok\r\n");
  if (rootPage == nullptr) rootPage = new astra::Menu("root");
  dbgStep("[a3] new Menu root ok\r\n");
  if (toolPage == nullptr) {
    pic_home.assign(pic_home_data, pic_home_data + sizeof(pic_home_data));
    pic_gear.assign(pic_gear_data, pic_gear_data + sizeof(pic_gear_data));
    pic_info.assign(pic_info_data, pic_info_data + sizeof(pic_info_data));
    pic_tool.assign(pic_tool_data, pic_tool_data + sizeof(pic_tool_data));
    toolPage = new astra::Menu("Tools", pic_tool);
  }
  dbgStep("[a4] toolpage ok\r\n");

  /* ---- 一级菜单 (磁贴页) ---- */
  astra::Menu* homeTile = new astra::Menu("Home", pic_home);
  astra::Menu* settingsTile = new astra::Menu("Settings", pic_gear);
  astra::Menu* aboutTile = new astra::Menu("About", pic_info);
  rootPage->addItem(homeTile);
  rootPage->addItem(settingsTile);
  rootPage->addItem(aboutTile);
  rootPage->addItem(toolPage);
  dbgStep("[a5] root items ok\r\n");

  /* ---- 二级菜单 (列表页) ---- */
  homeTile->addItem(new astra::Menu("-Status"));
  homeTile->addItem(new astra::Menu("-Uptime"));
  homeTile->addItem(new astra::Menu("-Memory"));

  settingsTile->addItem(new astra::Menu("-Brightness"));
  settingsTile->addItem(new astra::Menu("-Contrast"));
  settingsTile->addItem(new astra::Menu("-Reset"));

  aboutTile->addItem(new astra::Menu("-Astra UI"));
  aboutTile->addItem(new astra::Menu("-STM32F103"));
  aboutTile->addItem(new astra::Menu("-ST7735S"));

  toolPage->addItem(new astra::Menu("-Encoder"));
  toolPage->addItem(new astra::Menu("-LCD Test"));
  toolPage->addItem(new astra::Menu("-LED Blink"));
  toolPage->addItem(new astra::Menu("-SPI DMA"));
  toolPage->addItem(new astra::Menu("-Key Scan"));
  dbgStep("[a6] tool items ok\r\n");

  astraLauncher->init(rootPage);
  dbgStep("[a7] launcher init ok\r\n");
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
}
