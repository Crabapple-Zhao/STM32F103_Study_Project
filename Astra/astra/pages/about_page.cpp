#include "about_page.h"
#include "app_config.h"
#include "app_log.h"
#include "hal.h"
#include "hal_port.h"
#include "ui/element/page/item.h"
#include <stdio.h>

static void aboutEnter() {
    APP_LOG_INFO("about", "page_enter version=" APP_VERSION);
}

static void aboutExit() {
    APP_LOG_INFO("about", "page_exit");
}

static void drawAbout() {
    // Six native 8x16 lines fit the 108px content area below the top title bar.
    // RAM is the linked reservation (including heap/stack), as in the old status bar.
    AstraMemoryUsage usage;
    astraGetMemoryUsage(&usage);
    char line[24];
    HAL::drawEnglish(4, 16, "FW: " APP_VERSION);
    HAL::drawEnglish(4, 34, "MCU: " APP_TARGET_NAME);
    snprintf(line, sizeof(line), "RAM: %luB %lu%%",
             (unsigned long)usage.ramBytes, (unsigned long)usage.ramPercent);
    HAL::drawEnglish(4, 52, line);
    snprintf(line, sizeof(line), "ROM: %luB %lu%%",
             (unsigned long)usage.romBytes, (unsigned long)usage.romPercent);
    HAL::drawEnglish(4, 70, line);
    uint32_t seconds = HAL_GetTick() / 1000U;
    snprintf(line, sizeof(line), "Up: %02lu:%02lu:%02lu",
             (unsigned long)(seconds / 3600U),
             (unsigned long)((seconds / 60U) % 60U),
             (unsigned long)(seconds % 60U));
    HAL::drawEnglish(4, 88, line);
    snprintf(line, sizeof(line), "FPS: %u", (unsigned int)astraGetFps());
    HAL::drawEnglish(4, 106, line);
}

void AboutPage_AddToMenu(astra::Menu *menu) {
    if (menu != nullptr)
        menu->addItem(new astra::Menu("-About", drawAbout, aboutEnter, aboutExit));
}
