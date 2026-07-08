//
// Created by Fir on 2024/2/8.
//

#include <cstring>
#include <string.h>
#include "hal.h"

HAL *HAL::hal = nullptr;

HAL *HAL::get() {
  return hal;
}

bool HAL::check() {
  return hal != nullptr;
}

bool HAL::inject(HAL *_hal) {
  if (_hal == nullptr) {
    return false;
  }

  _hal->init();
  hal = _hal;
  return true;
}

void HAL::destroy() {
  if (hal == nullptr) return;

  delete hal;
  hal = nullptr;
}

/**
 * @brief log printer. 自动换行的日志输出
 *
 * @param _msg message want to print. 要输出的信息
 * @note cannot execute within a loop. 不能在循环内执行
 */
void HAL::_printInfo(std::string _msg) {
  static std::vector<std::string> _infoCache = {};
  static const uint8_t _max = getSystemConfig().screenHeight / getFontHeight();
  static const uint8_t _fontHeight = getFontHeight();

  if (_infoCache.size() >= _max) _infoCache.clear();
  _infoCache.push_back(_msg);

  canvasClear();
  setDrawType(2); //反色显示
  for (uint8_t i = 0; i < _infoCache.size(); i++) {
    drawEnglish(0, _fontHeight + i * (1 + _fontHeight), _infoCache[i]);
  }
  canvasUpdate();
  setDrawType(1); //回归实色显示
}

bool HAL::_getAnyKey() {
  for (int i = 0; i < key::KEY_NUM; i++) {
    if (getKey(static_cast<key::KEY_INDEX>(i))) return true;
  }
  return false;
}

/**
 * @brief key scanner default. 默认按键扫描函数
 *
 * @note 建议在定时器或主循环中以固定周期（约 5~10ms）调用 HAL::keyScan().
 * @note 修复了原框架中 CLICK 短按判定失效的 bug：
 *       原实现在进入 RELEASED 状态后才判断 _keyFilter，此时已被覆盖为 RELEASED，
 *       导致短按 CLICK 永远不会产生。这里用 _confirmKey 记录待确认的按键。
 */
void HAL::_keyScan() {
  static uint8_t _timeCnt = 0;
  static bool _lock = false;
  static key::KEY_FILTER _keyFilter = key::CHECKING;
  static key::KEY_INDEX _confirmKey = key::KEY_0;  //记录当前正在确认的按键
  switch (_keyFilter) {
    case key::CHECKING:
      if (getAnyKey()) {
        if (getKey(key::KEY_0)) { _keyFilter = key::KEY_0_CONFIRM; _confirmKey = key::KEY_0; }
        if (getKey(key::KEY_1)) { _keyFilter = key::KEY_1_CONFIRM; _confirmKey = key::KEY_1; }
      }
      _timeCnt = 0;
      _lock = false;
      break;
    case key::KEY_0_CONFIRM:
    case key::KEY_1_CONFIRM:
      //filter
      if (getAnyKey()) {
        if (!_lock) _lock = true;
        _timeCnt++;

        //timer 长按约 1s
        if (_timeCnt > 100) {
          if (getKey(_confirmKey)) {
            key[_confirmKey] = key::PRESS;
            key[(_confirmKey == key::KEY_0) ? key::KEY_1 : key::KEY_0] = key::RELEASE;
          }
          _timeCnt = 0;
          _lock = false;
          _keyFilter = key::RELEASED;
        }
      } else {
        if (_lock) {
          //短按释放，产生 CLICK
          key[_confirmKey] = key::CLICK;
          key[(_confirmKey == key::KEY_0) ? key::KEY_1 : key::KEY_0] = key::RELEASE;
          _keyFilter = key::RELEASED;
        } else {
          _keyFilter = key::CHECKING;
          key[key::KEY_0] = key[key::KEY_1] = key::RELEASE;
        }
      }
      break;
    case key::RELEASED:
      if (!getAnyKey()) _keyFilter = key::CHECKING;
      break;
    default: break;
  }
}

/**
 * @brief default key tester. 默认按键测试函数
 */
void HAL::_keyTest() {
  if (getAnyKey()) {
    for (uint8_t i = 0; i < key::KEY_NUM; i++) {
      if (key[i] == key::CLICK) {
        //do something when key clicked
        if (i == 0) break;
        if (i == 1) break;
      } else if (key[i] == key::PRESS) {
        //do something when key pressed
        if (i == 0) break;
        if (i == 1) break;
      }
    }
    memset(key, key::RELEASE, sizeof(key));
  }
}
