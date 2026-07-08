/**
 * @file    GuiLite_min.h — GuiLite 最小化渲染子集 (STM32F103 专用)
 *
 * 从 GuiLite v3.4 提取的核心渲染类型，排除以下内容以节省 ~5KB RAM：
 *   - c_fifo、消息系统 (GL_MSG_ENTRY, c_cmd_target)
 *   - 所有控件 (c_wnd, c_button, c_keyboard, c_list_box, c_waveform 等)
 *   - 底部静态变量定义 (#ifdef GUILITE_ON 段)
 *
 * 适配说明:
 *   - 字体数据格式: ROW-MAJOR 1bpp, MSB=左像素, 每字节=1行8像素
 *   - 颜色格式: GL_RGB = 0xAARRGGBB, LCD = RGB565
 *   - 无帧缓冲模式: 通过 EXTERNAL_GFX_OP 回调直接写 LCD
 */
#ifndef GUILITE_MIN_H
#define GUILITE_MIN_H

#define GUILITE_CORE_INCLUDE_DISPLAY_H  /* 防止 font_ascii_8x16.h 重复定义 LATTICE/FONT_INFO */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* 共享 GFX 接口类型（与 GuiLiteAdapter.cpp 共用） */
#include "gfx_types.h"

/* ======================== 颜色宏 ======================== */

#define GL_ARGB(a, r, g, b) (((unsigned int)(a) << 24) | ((unsigned int)(r) << 16) | ((unsigned int)(g) << 8) | (unsigned int)(b))
#define GL_RGB(r, g, b)     (0xFF000000 | ((unsigned int)(r) << 16) | ((unsigned int)(g) << 8) | (unsigned int)(b))
#define GL_RGB_R(rgb)       (((rgb) >> 16) & 0xFF)
#define GL_RGB_G(rgb)       (((rgb) >> 8) & 0xFF)
#define GL_RGB_B(rgb)       ((rgb) & 0xFF)

/* ARGB(32bit) -> RGB565(16bit): 正确提取 AARRGGBB 中的 R/G/B 通道 */
#define GL_RGB_32_to_16(r) ((((r)>>19)<<11) & 0xF800) | ((((r)>>10)<<5) & 0x07E0) | (((r)>>3) & 0x1F)
/* RGB565(16bit) -> ARGB(32bit) */
#define GL_RGB_16_to_32(r) (0xFF000000 | (((r)&0x1F)<<3) | (((r)&0x7E0)<<5) | (((r)&0xF800)<<8))

/* ======================== 对齐宏 ======================== */

#define ALIGN_HCENTER  0x00000000L
#define ALIGN_LEFT     0x01000000L
#define ALIGN_RIGHT    0x02000000L
#define ALIGN_VCENTER  0x00000000L
#define ALIGN_TOP      0x00100000L
#define ALIGN_BOTTOM   0x00200000L

#define ASSERT(c)      do { if (!(c)) { for (;;); } } while (0)
#pragma diag_suppress 111,1293
#define MAX(a,b)       (((a) > (b)) ? (a) : (b))
#define MIN(a,b)       (((a) < (b)) ? (a) : (b))

/* ======================== 字体类型 ======================== */

typedef struct {
    unsigned int         utf8_code;
    unsigned char        width;
    const unsigned char *pixel_gray_array;
} LATTICE;

typedef struct {
    unsigned char height;
    unsigned int  count;
    LATTICE     *lattice_array;
} FONT_INFO;

/* ======================== 几何类型 ======================== */

class c_rect {
public:
    c_rect() { m_left = m_top = m_right = m_bottom = -1; }
    c_rect(int left, int top, int width, int height) { set_rect(left, top, width, height); }

    void set_rect(int left, int top, int width, int height) {
        m_left = left; m_top = top;
        m_right = left + width - 1; m_bottom = top + height - 1;
    }
    bool pt_in_rect(int x, int y) const { return x >= m_left && x <= m_right && y >= m_top && y <= m_bottom; }
    int  width()  const { return m_right - m_left + 1; }
    int  height() const { return m_bottom - m_top + 1; }
    int  operator==(const c_rect& r) const {
        return (m_left == r.m_left) && (m_top == r.m_top) && (m_right == r.m_right) && (m_bottom == r.m_bottom);
    }

    int m_left, m_top, m_right, m_bottom;
};

/* ======================== 层级与显示接口 ======================== */

typedef enum { Z_ORDER_LEVEL_0, Z_ORDER_LEVEL_1, Z_ORDER_LEVEL_2, Z_ORDER_LEVEL_MAX } Z_ORDER_LEVEL;

enum FONT_TYPE { FONT_NULL, FONT_DEFAULT, FONT_CUSTOM1, FONT_CUSTOM2, FONT_CUSTOM3,
                 FONT_CUSTOM4, FONT_CUSTOM5, FONT_CUSTOM6, FONT_MAX };
enum COLOR_TYPE { COLOR_WND_FONT, COLOR_WND_NORMAL, COLOR_WND_PUSHED, COLOR_WND_FOCUS,
                  COLOR_WND_BORDER, COLOR_CUSTOME1, COLOR_CUSTOME2, COLOR_CUSTOME3,
                  COLOR_CUSTOME4, COLOR_MAX };

/* 层级与颜色枚举定义在上方 */

#define SURFACE_CNT_MAX 6

/* ======================== 主题管理 ======================== */

class c_theme {
public:
    static int add_font(FONT_TYPE index, FONT_INFO* font) {
        if (index >= FONT_MAX) { ASSERT(false); return -1; }
        s_font_map[index] = font; return 0;
    }
    static int add_color(COLOR_TYPE index, unsigned int color) {
        if (index >= COLOR_MAX) { ASSERT(false); return -1; }
        s_color_map[index] = color; return 0;
    }
    static FONT_INFO* get_font(FONT_TYPE index) {
        if (index >= FONT_MAX) { ASSERT(false); return 0; }
        return s_font_map[index];
    }
    static unsigned int get_color(COLOR_TYPE index) {
        if (index >= COLOR_MAX) { ASSERT(false); return 0; }
        return s_color_map[index];
    }
private:
    static FONT_INFO*    s_font_map[FONT_MAX];
    static unsigned int  s_color_map[COLOR_MAX];
};

/* ======================== Surface 层（画布）======================== */

class c_display;
class c_layer {
public:
    c_layer() { fb = 0; }
    void  *fb;
    c_rect rect;
};

class c_surface {
    friend class c_display;
public:
    c_surface(unsigned int w, unsigned int h, unsigned int cb,
              Z_ORDER_LEVEL max_z = Z_ORDER_LEVEL_0, c_rect overlap = c_rect())
        : m_width(w), m_height(h), m_color_bytes(cb), m_fb(0), m_is_active(false),
          m_top_zorder(Z_ORDER_LEVEL_0), m_phy_fb(0), m_phy_write_index(0),
          m_display(0), m_surface_cnt(0)
    {
        (overlap == c_rect()) ? set_surface(max_z, c_rect(0, 0, w - 1, h - 1))
                               : set_surface(max_z, overlap);
    }
    int  get_width()  { return m_width; }
    int  get_height() { return m_height; }

    virtual void draw_pixel(int x, int y, unsigned int rgb, unsigned int z_order);
    virtual void fill_rect(int x0, int y0, int x1, int y1, unsigned int rgb, unsigned int z_order);
    virtual void draw_line(int x0, int y0, int x1, int y1, unsigned int rgb, unsigned int z_order);
    virtual void draw_rect(int x0, int y0, int x1, int y1, unsigned int rgb, unsigned int z_order, int size);

protected:
    void set_surface(Z_ORDER_LEVEL max_zorder, c_rect layer_rect);
    virtual void draw_pixel_on_fb(int x, int y, unsigned int rgb);
    virtual void fill_rect_on_fb(int x0, int y0, int x1, int y1, unsigned int rgb);

    unsigned int           m_width, m_height, m_color_bytes;
    void                  *m_fb;
    bool                   m_is_active;
    Z_ORDER_LEVEL          m_top_zorder, m_max_zorder;
    void                  *m_phy_fb;
    int                   *m_phy_write_index;
    c_display             *m_display;
    int                    m_surface_cnt;
    c_layer                m_layers[Z_ORDER_LEVEL_MAX];
    struct EXTERNAL_GFX_OP *m_gfx_op;
};

/* ======================== 无帧缓冲 Surface ======================== */

class c_surface_no_fb : public c_surface {
    friend class c_display;
public:
    c_surface_no_fb(unsigned int w, unsigned int h, unsigned int cb,
                    struct EXTERNAL_GFX_OP* gfx_op,
                    Z_ORDER_LEVEL max_z = Z_ORDER_LEVEL_0, c_rect overlap = c_rect())
        : c_surface(w, h, cb, max_z, overlap), m_gfx_op(gfx_op) {}
protected:
    virtual void draw_pixel_on_fb(int x, int y, unsigned int rgb) {
        if (m_gfx_op && m_gfx_op->draw_pixel && m_is_active) m_gfx_op->draw_pixel(x, y, rgb);
    }
    virtual void fill_rect_on_fb(int x0, int y0, int x1, int y1, unsigned int rgb) {
        if (!m_gfx_op) return;
        if (m_gfx_op->fill_rect) { m_gfx_op->fill_rect(x0, y0, x1, y1, rgb); return; }
        if (m_gfx_op->draw_pixel && m_is_active)
            for (int y = y0; y <= y1; y++)
                for (int x = x0; x <= x1; x++) m_gfx_op->draw_pixel(x, y, rgb);
    }
    struct EXTERNAL_GFX_OP *m_gfx_op;
};

/* ======================== Display 管理器 ======================== */

class c_display {
    friend class c_surface;
public:
    inline c_display(void* phy_fb, int dw, int dh, int sw, int sh,
                     unsigned int cb, int surface_cnt, EXTERNAL_GFX_OP* gfx_op = 0)
        : m_width(dw), m_height(dh), m_color_bytes(cb), m_phy_fb(phy_fb),
          m_phy_read_index(0), m_phy_write_index(0),
          m_surface_cnt(surface_cnt), m_surface_index(0)
    {
        ASSERT(cb == 2 || cb == 4);
        ASSERT(m_surface_cnt <= SURFACE_CNT_MAX);
        memset(m_surface_group, 0, sizeof(m_surface_group));
        for (int i = 0; i < m_surface_cnt; i++) {
            m_surface_group[i] = phy_fb
                ? new c_surface(sw, sh, cb, Z_ORDER_LEVEL_0)
                : new c_surface_no_fb(sw, sh, cb, gfx_op, Z_ORDER_LEVEL_0);
            m_surface_group[i]->m_display = this;
            m_surface_group[i]->m_phy_fb = phy_fb;
            m_surface_group[i]->m_phy_write_index = &m_phy_write_index;
            m_surface_group[i]->m_is_active = true;
            m_surface_group[i]->m_surface_cnt = surface_cnt;
        }
    }
    inline c_surface* alloc_surface(Z_ORDER_LEVEL max_zorder, c_rect layer_rect = c_rect()) {
        if (m_surface_index >= m_surface_cnt) { ASSERT(false); return 0; }
        c_surface* s = m_surface_group[m_surface_index++];
        s->set_surface(max_zorder, layer_rect);
        return s;
    }
    int get_width()  { return m_width; }
    int get_height() { return m_height; }

private:
    int        m_width, m_height;
    unsigned int m_color_bytes;
    void      *m_phy_fb;
    int        m_phy_read_index, m_phy_write_index;
    int        m_surface_cnt, m_surface_index;
    c_surface *m_surface_group[SURFACE_CNT_MAX];
};

/* ======================== Surface 方法实现 ======================== */

void c_surface::set_surface(Z_ORDER_LEVEL max_zorder, c_rect layer_rect) {
    m_max_zorder = max_zorder;
    m_top_zorder = Z_ORDER_LEVEL_0;
    memset(m_layers, 0, sizeof(m_layers));
    if (m_surface_cnt > 1)
        m_fb = calloc(m_width * m_height, m_color_bytes);
    for (int i = Z_ORDER_LEVEL_0; i < m_max_zorder; i++) {
        ASSERT(m_layers[i].fb = calloc(layer_rect.width() * layer_rect.height(), m_color_bytes));
        m_layers[i].rect = layer_rect;
    }
}

void c_surface::draw_pixel(int x, int y, unsigned int rgb, unsigned int z_order) {
    if (x >= m_width || y >= m_height || x < 0 || y < 0) return;
    if (z_order > (unsigned int)m_max_zorder) { ASSERT(false); return; }
    if (z_order == m_max_zorder) return draw_pixel_on_fb(x, y, rgb);

    if (z_order > (unsigned int)m_top_zorder) m_top_zorder = (Z_ORDER_LEVEL)z_order;
    if (m_layers[z_order].rect.pt_in_rect(x, y)) {
        c_rect lr = m_layers[z_order].rect;
        if (m_color_bytes == 4)
            ((unsigned int*)(m_layers[z_order].fb))[(x - lr.m_left) + (y - lr.m_top) * lr.width()] = rgb;
        else
            ((unsigned short*)(m_layers[z_order].fb))[(x - lr.m_left) + (y - lr.m_top) * lr.width()] = GL_RGB_32_to_16(rgb);
    }
    if (z_order == m_top_zorder) return draw_pixel_on_fb(x, y, rgb);

    bool ov = false;
    for (unsigned int z = Z_ORDER_LEVEL_MAX - 1; z > z_order; z--)
        if (m_layers[z].rect.pt_in_rect(x, y)) { ov = true; break; }
    if (!ov) draw_pixel_on_fb(x, y, rgb);
}

void c_surface::fill_rect(int x0, int y0, int x1, int y1, unsigned int rgb, unsigned int z_order) {
    x0 = (x0 < 0) ? 0 : x0;  y0 = (y0 < 0) ? 0 : y0;
    x1 = (x1 > (int)m_width - 1) ? m_width - 1 : x1;
    y1 = (y1 > (int)m_height - 1) ? m_height - 1 : y1;
    if (z_order == m_max_zorder) return fill_rect_on_fb(x0, y0, x1, y1, rgb);
    if (z_order == m_top_zorder) {
        c_rect lr = m_layers[z_order].rect;
        unsigned short rgb16 = GL_RGB_32_to_16(rgb);
        for (int y = y0; y <= y1; y++)
            for (int x = x0; x <= x1; x++)
                if (lr.pt_in_rect(x, y))
                    (m_color_bytes == 4)
                        ? (void)(((unsigned int*) (m_layers[z_order].fb))[(x-lr.m_left)+(y-lr.m_top)*lr.width()] = rgb)
                        : (void)(((unsigned short*)(m_layers[z_order].fb))[(x-lr.m_left)+(y-lr.m_top)*lr.width()] = rgb16);
        return fill_rect_on_fb(x0, y0, x1, y1, rgb);
    }
    fill_rect_on_fb(x0, y0, x1, y1, rgb);
}

void c_surface::draw_line(int x0, int y0, int x1, int y1, unsigned int rgb, unsigned int z_order) {
    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx - dy, e2;
    for (;;) {
        draw_pixel(x0, y0, rgb, z_order);
        if (x0 == x1 && y0 == y1) break;
        e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x0 += sx; }
        if (e2 < dx) { err += dx; y0 += sy; }
    }
}

void c_surface::draw_rect(int x0, int y0, int x1, int y1, unsigned int rgb, unsigned int z_order, int size) {
    fill_rect(x0, y0, x1, y0 + size - 1, rgb, z_order);
    fill_rect(x0, y1 - size + 1, x1, y1, rgb, z_order);
    fill_rect(x0, y0, x0 + size - 1, y1, rgb, z_order);
    fill_rect(x1 - size + 1, y0, x1, y1, rgb, z_order);
}

void c_surface::draw_pixel_on_fb(int x, int y, unsigned int rgb) {
    if (!m_gfx_op) return;
    if (m_gfx_op->draw_pixel && m_is_active) m_gfx_op->draw_pixel(x, y, rgb);
    if (!m_fb) return;
    if (m_color_bytes == 4)
        ((unsigned int*)m_fb)[y * m_width + x] = rgb;
    else if (m_color_bytes == 2)
        ((unsigned short*)m_fb)[y * m_width + x] = GL_RGB_32_to_16(rgb);
}

void c_surface::fill_rect_on_fb(int x0, int y0, int x1, int y1, unsigned int rgb) {
    if (!m_gfx_op) return;
    if (m_gfx_op->fill_rect) { m_gfx_op->fill_rect(x0, y0, x1, y1, rgb); return; }
    if (m_gfx_op->draw_pixel && m_is_active)
        for (int y = y0; y <= y1; y++)
            for (int x = x0; x <= x1; x++) m_gfx_op->draw_pixel(x, y, rgb);
    if (!m_fb) return;
    if (m_color_bytes == 4) {
        unsigned int* fb;
        for (int y = y0; y <= y1; y++) {
            fb = &((unsigned int*)m_fb)[y * m_width + x0];
            for (int x = x0; x <= x1; x++) *fb++ = rgb;
        }
    } else if (m_color_bytes == 2) {
        unsigned short* fb;
        rgb = GL_RGB_32_to_16(rgb);
        for (int y = y0; y <= y1; y++) {
            fb = &((unsigned short*)m_fb)[y * m_width + x0];
            for (int x = x0; x <= x1; x++) *fb++ = rgb;
        }
    }
}

/* ======================== 文字渲染引擎 ======================== */

#define BUFFER_LEN 16

class c_word {
public:
    static void draw_string(c_surface* surface, int z_order, const char* s, int x, int y,
                            FONT_INFO* font, unsigned int font_color, unsigned int bg_color,
                            unsigned int align_type = ALIGN_LEFT) {
        if (!s) return;
        int offset = 0; unsigned int utf8_code;
        while (*s) {
            s += get_utf8_code(s, utf8_code);
            offset += draw_single_char(surface, z_order, utf8_code, x + offset, y, font, font_color, bg_color);
        }
    }
    static void draw_string_in_rect(c_surface* surface, int z_order, const char* s, c_rect rect,
                                    FONT_INFO* font, unsigned int fc, unsigned int bc,
                                    unsigned int align = ALIGN_LEFT) {
        if (!s) return;
        int px, py; get_string_pos(s, font, rect, align, px, py);
        draw_string(surface, z_order, s, rect.m_left + px, rect.m_top + py, font, fc, bc, ALIGN_LEFT);
    }
    static void draw_value(c_surface* surface, int z_order, int val, int dot, int x, int y,
                            FONT_INFO* font, unsigned int fc, unsigned int bc,
                            unsigned int align = ALIGN_LEFT) {
        char buf[BUFFER_LEN]; value_2_string(val, dot, buf, BUFFER_LEN);
        draw_string(surface, z_order, buf, x, y, font, fc, bc, align);
    }
    static int get_str_size(const char* s, FONT_INFO* font, int& w, int& h) {
        if (!s || !font) { w = h = 0; return -1; }
        int lw = 0; unsigned int code;
        while (*s) {
            int nb = get_utf8_code(s, code);
            const LATTICE* p = get_lattice(font, code);
            lw += p ? p->width : font->height;
            s += nb;
        }
        w = lw; h = font->height; return 0;
    }

private:
    static int get_utf8_code(const char* s, unsigned int& code) {
        if (!(s[0] & 0x80))             { code = s[0]; return 1; }
        if ((s[0]&0xE0)==0xC0 && (s[1]&0xC0)==0x80) { code=(s[0]&0x1F)<<6|(s[1]&0x3F); return 2; }
        if ((s[0]&0xF0)==0xE0 && (s[1]&0xC0)==0x80 && (s[2]&0xC0)==0x80)
                                            { code=(s[0]&0x0F)<<12|((s[1]&0x3F)<<6)|(s[2]&0x3F); return 3; }
        if ((s[0]&0xF8)==0xF0 && (s[1]&0xC0)==0x80 && (s[2]&0xC0)==0x80 && (s[3]&0xC0)==0x80)
                                            { code=(s[0]&0x07)<<18|((s[1]&0x3F)<<12)|((s[2]&0x3F)<<6)|(s[3]&0x3F); return 4; }
        code = '?'; return 1;
    }
    static const LATTICE* get_lattice(FONT_INFO* font, unsigned int code) {
        if (!font || !font->lattice_array) return 0;
        LATTICE* end = font->lattice_array + font->count;
        for (LATTICE* p = font->lattice_array; p < end; p++)
            if (p->utf8_code == code) return p;
        return 0;
    }
    static int draw_single_char(c_surface* s, int zo, unsigned int code, int x, int y,
                                FONT_INFO* f, unsigned int fg, unsigned int bg) {
        const LATTICE* pl = get_lattice(f, code);
        if (pl) { draw_lattice(s, zo, x, y, pl->width, f->height, pl->pixel_gray_array, fg, bg); return pl->width; }
        return f->height;
    }

    /**
     * 渲染字体点阵 — 支持 ROW-MAJOR 1bpp 格式
     * 每字节代表一行像素，MSB(bit7)=最左像素，LSB(bit0)=最右像素
     * 例如 8x16 字体: 16字节/字符，data[row] 的每一位对应一个像素
     */
    static void draw_lattice(c_surface* s, int zo, int x, int y, int cw, int ch,
                             const unsigned char* data, unsigned int fg, unsigned int bg) {
        for (int row = 0; row < ch; row++) {
            unsigned char line = data[row];
            for (int col = 0; col < cw; col++) {
                if (line & (1 << (cw - 1 - col)))
                    s->draw_pixel(x + col, y + row, fg, zo);
                else if (bg != GL_ARGB(0, 0, 0, 0))
                    s->draw_pixel(x + col, y + row, bg, zo);
            }
        }
    }
    static void value_2_string(int value, int dot, char* buf, int len) {
        memset(buf, 0, len);
        switch (dot) {
        case 0: sprintf(buf, "%d", value); break;
        case 1: sprintf(buf, "%.1f", value * 1.0 / 10); break;
        case 2: sprintf(buf, "%.2f", value * 1.0 / 100); break;
        case 3: sprintf(buf, "%.3f", value * 1.0 / 1000); break;
        default: ASSERT(false); break;
        }
    }
    static void get_string_pos(const char* s, FONT_INFO* font, c_rect rect, unsigned int align, int& x, int& y) {
        int sw, sh; get_str_size(s, font, sw, sh);
        switch (align & 0x03000000) {
        case ALIGN_LEFT:  x = 0; break;
        case ALIGN_RIGHT: x = rect.width() - sw; break;
        default:         x = (rect.width() - sw) / 2; break;
        }
        switch (align & 0x00300000) {
        case ALIGN_TOP:    y = 0; break;
        case ALIGN_BOTTOM: y = rect.height() - sh; break;
        default:           y = (rect.height() - sh) / 2; break;
        }
    }
};

/* ======================== 静态成员定义 ======================== */
FONT_INFO*    c_theme::s_font_map[FONT_MAX]  = {0};
unsigned int  c_theme::s_color_map[COLOR_MAX] = {0};

#pragma diag_default 111,1293
#endif /* GUILITE_MIN_H */
