//
// Astra UI 图标资源接口
// Bitmap: 1bpp 点阵 (数据指针 + 宽高 + 字节数), 尺寸信息随资源自带
// Icon:   菜单磁贴图标 (普通图必选, 选中图可选, 为空时回退为放大普通图)
//
#pragma once
#ifndef ASTRA_CORE_SRC_ASTRA_ASTRA_ICON_H_
#define ASTRA_CORE_SRC_ASTRA_ASTRA_ICON_H_

#include <stdint.h>

namespace astra {

/* 1bpp 行主序 LSB first 点阵 */
struct Bitmap {
  const uint8_t *data;  // 点阵数据
  uint8_t width;       // 像素宽
  uint8_t height;      // 像素高
  uint16_t size;       // 字节数, 须满足 >= ((width+7)/8)*height

  /* 长度校验: 拦截数组长度与宽高不匹配导致的越界读取 */
  bool valid() const {
    return data != nullptr && size >= (uint16_t)(((width + 7) / 8) * height);
  }
};

/* 菜单磁贴图标: selected 为空时按全局布局尺寸放大普通图 */
struct Icon {
  const Bitmap *normal;
  const Bitmap *selected;  // 可选, 独立选中图
};

}  // namespace astra

#endif  // ASTRA_CORE_SRC_ASTRA_ASTRA_ICON_H_
