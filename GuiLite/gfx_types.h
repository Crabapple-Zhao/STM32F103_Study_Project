/**
 * @file    gfx_types.h — GUI 图形操作接口类型
 *
 * 定义 EXTERNAL_GFX_OP 结构体，作为 GuiLite 与底层显示驱动的桥梁。
 * GuiLite_min.h 和 GuiLiteAdapter.cpp 共用此类型定义。
 */
#ifndef GFX_TYPES_H
#define GFX_TYPES_H

struct EXTERNAL_GFX_OP {
    void (*draw_pixel)(int x, int y, unsigned int rgb);
    void (*fill_rect)(int x0, int y0, int x1, int y1, unsigned int rgb);
};

#endif /* GFX_TYPES_H */
