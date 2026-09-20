#include "display_page.h"
#include "app_log.h"
#include "hal.h"
#include "lcd.h"
#include "ui/element/page/item.h"

static uint8_t selection = 0;

static void displayEnter() {
    selection = LCD_IsFlipped();
    APP_LOG_INFO("display", "page_enter");
}

static void displayExit() { APP_LOG_INFO("display", "page_exit"); }

static bool displayKey(key::KEY_INDEX index, key::KEY_ACTION action) {
    if (action == key::CLICK) {
        selection = index == key::KEY_1 ? 1U : 0U;
        return true;
    }
    if (action == key::PRESS && index == key::KEY_1) {
        LCD_SetFlipped(selection);
        if (LCD_IsFlipped()) APP_LOG_INFO("display", "orientation=flipped");
        else APP_LOG_INFO("display", "orientation=normal");
        return true;
    }
    return false; // Long press retains the standard back navigation.
}

static void drawDisplay() {
    HAL::drawEnglish((APP_DISPLAY_WIDTH - 11 * 8) / 2, 24, "Orientation");
    HAL::drawHLine((APP_DISPLAY_WIDTH - 108) / 2, 32, 108);
    const int left = (APP_DISPLAY_WIDTH - 12 * 8) / 2;
    HAL::drawEnglish(left, 52, selection == 0 ? "> Normal" : "  Normal");
    HAL::drawEnglish(left, 72, selection == 1 ? "> Flipped" : "  Flipped");
    HAL::drawEnglish(left + 11 * 8, LCD_IsFlipped() ? 72 : 52, "*");
    HAL::drawEnglish((APP_DISPLAY_WIDTH - 14 * 8) / 2, 102, "Press to apply");
}

void DisplayPage_AddToMenu(astra::Menu *menu) {
    if (menu != nullptr)
        menu->addItem(new astra::Menu("-Display", drawDisplay,
                                     displayEnter, displayExit, displayKey));
}
