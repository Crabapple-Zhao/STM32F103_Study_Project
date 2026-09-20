//
// Created by Fir on 2024/1/25.
//
// UI 风格配置：8x16 字体；屏幕尺寸在 User/display_config.h 中定义
//

#pragma once
#ifndef ASTRA_CORE_SRC_SYSTEM_H_
#define ASTRA_CORE_SRC_SYSTEM_H_

#include "cstdint"
#include <stdint.h>

namespace astra {
/**
 * @brief config of astra ui. astra ui的配置结构体
 * @note 字体成员默认为 nullptr，需在 astraCoreInit 之前由用户赋值，
 *       例如 getUIConfig().mainFont = &myFontArray; 字体格式由 HAL 实现的
 *       setFont / drawEnglish / drawChinese 决定（原框架使用 u8g2 字体）。
 */
struct config {
  float tileAnimationSpeed = 95;
  float listAnimationSpeed = 90;
  float selectorYAnimationSpeed = 90;
  float selectorXAnimationSpeed = 95;
  float selectorWidthAnimationSpeed = 95;
  float selectorHeightAnimationSpeed = 90;
  float windowAnimationSpeed = 25;
  float sideBarAnimationSpeed = 15;
  float fadeAnimationSpeed = 100;
  float cameraAnimationSpeed = 95;
  float logoAnimationSpeed = 70;

  bool tileUnfold = true;
  bool listUnfold = true;

  bool tileLoop = true;
  bool menuLoop = false;

  bool backgroundBlur = true;
  bool lightMode = false;

  float listBarWeight = 5;
  float listTextHeight = 12;  // 适配 16px 字体: 16 - listTextMargin(4) = 12
  float listTextMargin = 4; //文字边距
  float listLineHeight = 16;
  float selectorRadius = 0.5f;
  float selectorMargin = 4; //选择框与文字左边距
  float selectorTopMargin = 2; //选择框与文字上边距

  uint8_t listPageTurningMode = 1; //0: 翻页模式 1: 滚动模式

  float tilePicWidth = 30;
  float tilePicHeight = 30;
  float tilePicMargin = 8;
  float tileSelectedPicWidth = 36;
  float tileSelectedPicHeight = 36;
  float tilePicTopMargin = 26; //图标上边距
  float tileArrowWidth = 6;
  float tileArrowMargin = 4; //箭头边距

  //相对于 UI 内容区域底部的边距，横屏内容区域高 108px
  float tileDottedLineBottomMargin = 28; //虚线下边距
  float tileArrowBottomMargin = 8; //箭头下边距
  float tileTextBottomMargin = 22; //标题下边距

  float tileBarHeight = 2; //磁贴进度条高度

  float tileSelectBoxLineLength = 5;  //磁贴选择框线长
  float tileSelectBoxMargin = 3; //选择框边距
  float tileSelectBoxWidth = tileSelectBoxMargin * 2 + tileSelectedPicWidth; //选择框宽
  float tileSelectBoxHeight = tileSelectBoxMargin * 2 + tileSelectedPicHeight; //选择框高
  float tileTitleHeight = 16; //磁贴标题高度 (适配 8x16 字体)

  float tileBtnMargin = 16; //按钮边距

  float popMargin = 4; //弹窗边距
  float popRadius = 2; //弹窗圆角半径
  float popSpeed = 90; //弹窗动画速度

  float logoStarLength = 2; //logo星星长度
  float logoTextHeight = 16; //logo文字高度 (适配 8x16 字体)
  float logoCopyRightHeight = 16; //logo版权文字高度
  uint8_t logoStarNum = 16; //logo星星数量

  //字体指针，默认为空，需用户在初始化时赋值（指向字模数组）
  const uint8_t *logoTitleFont = nullptr;
  const uint8_t *logoCopyRightFont = nullptr;
  const uint8_t *mainFont = nullptr;
};

config &getUIConfig();
}
#endif //ASTRA_CORE_SRC_SYSTEM_H_
