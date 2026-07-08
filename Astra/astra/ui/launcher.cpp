//
// Created by Fir on 2024/2/2.
//

#include "launcher.h"
#include <cmath>

namespace astra {

void Launcher::popInfo(std::string _info, uint16_t _time) {
  static const uint64_t beginTime = this->time;
  static bool onRender = true;

  while (onRender) {
    time++;

    static float wPop = HAL::getFontWidth(_info) + 2 * getUIConfig().popMargin;  //宽度
    static float hPop = HAL::getFontHeight() + 2 * getUIConfig().popMargin;  //高度

    static float yPop = 0 - hPop - 8; //从屏幕上方滑入
    static float yPopTrg = 0;
    if (time - beginTime < _time) yPopTrg = (HAL::getSystemConfig().screenHeight - hPop) / 3;  //目标位置 中间偏上
    else yPopTrg = 0 - hPop - 8;  //滑出

    static float xPop = (HAL::getSystemConfig().screenWeight - wPop) / 2;  //居中

    HAL::canvasClear();
    /*渲染一帧*/
    currentPage->render(camera->getPosition());
    selector->render(camera->getPosition());
    camera->update(currentPage, selector);
    /*渲染一帧*/

    HAL::setDrawType(0);
    HAL::drawRBox(xPop - 4, yPop - 4, wPop + 8, hPop + 8, getUIConfig().popRadius + 2);
    HAL::setDrawType(1);  //反色显示
    HAL::drawRFrame(xPop - 1, yPop - 1, wPop + 2, hPop + 2, getUIConfig().popRadius);  //绘制一个圆角矩形
    HAL::drawEnglish(xPop + getUIConfig().popMargin, yPop + getUIConfig().popMargin + HAL::getFontHeight(), _info);  //绘制文字

    HAL::canvasUpdate();

    animation(&yPop, yPopTrg, getUIConfig().popSpeed);  //动画

    //todo 这里条件可以加上一个如果按键按下 就退出
    //修复: 浮点精确比较可能永不满足, 改用容差判断
    if (time - beginTime >= _time && std::fabs(yPop - (0 - hPop - 8)) < 1.0f) onRender = false;  //退出条件
  }
}

void Launcher::init(Menu *_rootPage) {
  currentPage = _rootPage;

  camera = new Camera(0, 0);
  _rootPage->init(camera->getPosition());

  selector = new Selector();
  selector->inject(_rootPage);
  selector->go(_rootPage->selectIndex);
}

/**
 * @brief 打开选中的页面
 *
 * @return 是否成功打开
 * @warning 仅可调用一次
 */
bool Launcher::open() {
  //todo 打开和关闭都还没写完 应该还漏掉了一部分内容

  //如果当前页面指向的当前item没有后继 那就返回false
  if (currentPage->getNext() == nullptr) return false;
  if (currentPage->getNext()->getItemNum() == 0) return false;

  currentPage->deInit();  //先析构（退场动画）再挪动指针

  currentPage = currentPage->getNext();
  currentPage->init(camera->getPosition());

  selector->inject(currentPage);
  //selector->go(currentPage->selectIndex);

  return true;
}

/**
 * @brief 关闭选中的页面
 *
 * @return 是否成功关闭
 * @warning 仅可调用一次
 */
bool Launcher::close() {
  if (currentPage->getPreview() == nullptr) return false;
  if (currentPage->getPreview()->getItemNum() == 0) return false;

  currentPage->deInit();  //先析构（退场动画）再挪动指针

  currentPage = currentPage->getPreview();
  currentPage->init(camera->getPosition());

  selector->inject(currentPage);
  //selector->go(currentPage->selectIndex);

  return true;
}

void Launcher::update() {
  HAL::canvasClear();

  currentPage->render(camera->getPosition());
  selector->render(camera->getPosition());
  camera->update(currentPage, selector);

  //按键扫描与菜单导航
  //KEY_0 短按=上一个  KEY_1 短按=下一个
  //KEY_0 长按=返回上一级  KEY_1 长按=打开选中项
  HAL::keyScan();
  if (HAL::getAnyKey()) {
    uint8_t num = currentPage->getItemNum();
    for (uint8_t i = 0; i < key::KEY_NUM; i++) {
      key::KEY_INDEX idx = static_cast<key::KEY_INDEX>(i);
      key::KEY_ACTION act = HAL::getKeyAction(idx);
      if (act == key::CLICK) {
        if (num == 0) continue;
        if (i == key::KEY_0) {  //上一个
          if (currentPage->selectIndex > 0) currentPage->selectIndex--;
          else if (getUIConfig().menuLoop) currentPage->selectIndex = num - 1;
          selector->go(currentPage->selectIndex);
        } else if (i == key::KEY_1) {  //下一个
          if (currentPage->selectIndex < num - 1) currentPage->selectIndex++;
          else if (getUIConfig().menuLoop) currentPage->selectIndex = 0;
          selector->go(currentPage->selectIndex);
        }
      } else if (act == key::PRESS) {
        if (i == key::KEY_0) close();   //返回上一级
        else if (i == key::KEY_1) open();  //打开选中项
      }
    }
    HAL::clearKeyAction();
  }

  HAL::canvasUpdate();

  time++;
}
}
