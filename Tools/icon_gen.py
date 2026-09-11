#!/usr/bin/env python3
"""Astra UI 菜单图标素材工具 (正式入口, 取代 tmp/ 临时脚本).

素材源:   Tools/icons/icons.json   ('#'=字形, '.'=背景, 行等宽)
生成目标: Astra/astra/astra_icons.h 的 pic_*_data 数组
          (Bitmap/Icon 资源封装定义在头文件中手工维护, 本工具不改写)

编码规则 (与固件 drawScaledBitmap 解码一致):
  - 1bpp, 行主序, LSB first
  - 字形像素 '#' -> 0, 背景 '.' -> 1
  - 行尾 padding 位 (x >= width) -> 0

用法:
  python Tools/icon_gen.py             # 校验: JSON 与头文件是否一致
  python Tools/icon_gen.py --write     # 将 JSON 重新生成到头文件
  python Tools/icon_gen.py --preview   # 生成 Tools/icon_preview.png 预览
"""

import argparse
import json
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
ICONS_JSON = ROOT / 'Tools' / 'icons' / 'icons.json'
ICONS_HEADER = ROOT / 'Astra' / 'astra' / 'astra_icons.h'
PREVIEW_PNG = ROOT / 'tools' / 'icon_preview.png'

# JSON 图标名 -> 头文件数组符号 (settings 沿用历史命名 tool)
SYMBOLS = {
    'home': ('pic_home_data', 'pic_home_selected_data'),
    'sensor': ('pic_sensor_data', 'pic_sensor_selected_data'),
    'settings': ('pic_tool_data', None),
}

NORMAL_SIZE = 30    # 普通图标边长 (config.h tilePicWidth/Height)
SELECTED_SIZE = 36  # 独立选中图标边长 (config.h tileSelectedPicWidth/Height)


def encode(rows):
    """ASCII 点阵 -> 1bpp 字节数组 (行主序 LSB first, padding 位清零)."""
    height = len(rows)
    width = len(rows[0])
    data = []
    for y in range(height):
        for xb in range(0, width, 8):
            glyph = 0
            for i in range(8):
                if xb + i < width and rows[y][xb + i] == '#':
                    glyph |= 1 << i
            mask = (1 << min(8, width - xb)) - 1  # 有效列掩码, padding 位为 0
            data.append((~glyph) & mask)
    return bytes(data)


def decode(data, width):
    """字节数组 -> ASCII 点阵 (与 encode 互逆, 用于回读校验)."""
    bpr = (width + 7) // 8
    height = len(data) // bpr
    rows = []
    for y in range(height):
        row = ''
        for x in range(width):
            bit = (data[y * bpr + x // 8] >> (x % 8)) & 1
            row += '.' if bit else '#'
        rows.append(row)
    return rows


def load_icons():
    """读取并校验 JSON 素材格式."""
    doc = json.loads(ICONS_JSON.read_text(encoding='utf-8'))
    icons = {}
    for entry in doc['icons']:
        name = entry['name']
        if name not in SYMBOLS:
            print(f"  [WARN] {name}: 未在 SYMBOLS 中登记, 需要 Bitmap/Icon 封装后才能用于菜单")
        for variant in ('normal', 'selected'):
            rows = entry.get(variant)
            if rows is None:
                continue
            widths = {len(r) for r in rows}
            if len(widths) != 1:
                sys.exit(f"[FAIL] {name}.{variant}: 行宽不一致 {sorted(widths)}")
            width = widths.pop()
            expected = NORMAL_SIZE if variant == 'normal' else SELECTED_SIZE
            if (width, len(rows)) != (expected, expected):
                print(f"  [WARN] {name}.{variant}: {width}x{len(rows)} (布局配置为 {expected}x{expected})")
            bad = {ch for r in rows for ch in r} - {'#', '.'}
            if bad:
                sys.exit(f"[FAIL] {name}.{variant}: 含非法字符 {bad}")
            icons[(name, variant)] = rows
    return icons


def read_header():
    return ICONS_HEADER.read_text(encoding='utf-8')


def header_array(header, symbol):
    m = re.search(r'static const uint8_t ' + symbol + r'\[\] = \{(.*?)\};',
                  header, re.S)
    if m is None:
        return None
    return bytes(int(v, 16) for v in re.findall(r'0x([0-9A-Fa-f]{2})', m.group(1)))


def format_array(symbol, data):
    body = '\n'.join('    ' + ', '.join(f'0x{v:02X}' for v in data[i:i + 16]) + ','
                     for i in range(0, len(data), 16))
    return f'static const uint8_t {symbol}[] = {{\n{body}\n}};'


def check(icons):
    """JSON 与头文件逐字节比对."""
    header = read_header()
    all_ok = True
    for (name, variant), rows in sorted(icons.items()):
        symbol = SYMBOLS.get(name, (name, name + '_selected'))[0 if variant == 'normal' else 1]
        if symbol is None:
            continue
        current = header_array(header, symbol)
        encoded = encode(rows)
        if current is None:
            print(f"  [MISS] {name}.{variant}: 头文件缺少 {symbol}, 用 --write 生成")
            all_ok = False
        elif current != encoded:
            diff = sum(a != b for a, b in zip(current, encoded))
            print(f"  [DIFF] {name}.{variant} ({symbol}): {diff} 字节不一致, 用 --write 同步")
            all_ok = False
        else:
            # 回读校验: 编码可逆
            if decode(current, len(rows[0])) != rows:
                print(f"  [FAIL] {name}.{variant}: 解码回读不匹配")
                all_ok = False
            else:
                print(f"  [ OK ] {name}.{variant} ({symbol}): {len(encoded)} 字节一致")
    return all_ok


def write(icons):
    """将 JSON 生成结果写回头文件 (只替换数组体, 资源封装定义不动)."""
    header = read_header()
    for (name, variant), rows in sorted(icons.items()):
        symbol = SYMBOLS.get(name, (name, name + '_selected'))[0 if variant == 'normal' else 1]
        if symbol is None:
            continue
        encoded = encode(rows)
        if decode(encoded, len(rows[0])) != rows:
            sys.exit(f"[FAIL] {name}.{variant}: 编码回读自检失败, 中止写入")
        declaration = format_array(symbol, encoded)
        pattern = r'static const uint8_t ' + symbol + r'\[\] = \{.*?\};'
        if re.search(pattern, header, re.S):
            header = re.sub(pattern, lambda m: declaration, header, count=1, flags=re.S)
        else:
            # 新增图标: 追加到数组区末尾 (Bitmap/Icon 封装需手工补充)
            anchor = '\n/* 点阵资源: 尺寸与字节数随资源自带'
            header = header.replace(anchor, '\n' + declaration + anchor, 1)
        print(f"  [WRITE] {symbol}: {len(encoded)} 字节")
    ICONS_HEADER.write_text(header, encoding='utf-8')


def preview(icons):
    """生成预览图: 原尺寸 + 固件同款最近邻放大 (无独立选中图的回退效果)."""
    try:
        from PIL import Image, ImageDraw
    except ImportError:
        sys.exit('[FAIL] 需要安装 Pillow: pip install Pillow')

    names = [name for (name, _v) in icons if _v == 'normal']
    scale, pad = 6, 24
    w = pad * 2 + (NORMAL_SIZE + SELECTED_SIZE + 40) * scale
    h = pad * 2 + len(names) * (SELECTED_SIZE * scale + 44)
    canvas = Image.new('RGB', (w, h), '#202020')
    draw = ImageDraw.Draw(canvas)

    for row, name in enumerate(names):
        normal = decode(encode(icons[(name, 'normal')]), NORMAL_SIZE)
        selected = icons.get((name, 'selected'))
        top = pad + row * (SELECTED_SIZE * scale + 44)
        for col, (label, rows, size) in enumerate([
                (f'{name} {NORMAL_SIZE}x{NORMAL_SIZE}', normal, NORMAL_SIZE),
                (f'{name} sel {SELECTED_SIZE}x{SELECTED_SIZE}',
                 decode(encode(selected), SELECTED_SIZE) if selected else None, SELECTED_SIZE)]):
            left = pad + col * (NORMAL_SIZE * scale + 140)
            if rows is None:  # 无独立选中图: 固件回退为放大普通图
                rows = ['' for _ in range(SELECTED_SIZE)]
                for y in range(SELECTED_SIZE):
                    rows[y] = ''.join(normal[y * NORMAL_SIZE // SELECTED_SIZE]
                                      [x * NORMAL_SIZE // SELECTED_SIZE]
                                      for x in range(SELECTED_SIZE))
            draw.text((left, top), f'{label} (x{scale})', fill='white')
            im = Image.new('RGB', (size, size), (255, 255, 255))
            for y, r in enumerate(rows):
                for x, ch in enumerate(r):
                    if ch == '#':
                        im.putpixel((x, y), (0, 0, 0))
            canvas.paste(im.resize((size * scale, size * scale), Image.Resampling.NEAREST),
                         (left, top + 22))
    PREVIEW_PNG.parent.mkdir(exist_ok=True)
    canvas.save(PREVIEW_PNG)
    print(f'  [ OK ] 预览已生成: {PREVIEW_PNG.relative_to(ROOT)}')


def main():
    parser = argparse.ArgumentParser(description='Astra UI 图标素材工具')
    parser.add_argument('--write', action='store_true', help='将 JSON 生成到头文件')
    parser.add_argument('--preview', action='store_true', help='生成预览图')
    args = parser.parse_args()

    print(f'素材源: {ICONS_JSON.relative_to(ROOT)}')
    print(f'目标:   {ICONS_HEADER.relative_to(ROOT)}')
    icons = load_icons()

    if args.write:
        write(icons)
    ok = check(icons)
    if args.preview:
        preview(icons)
    if not ok:
        sys.exit(1)


if __name__ == '__main__':
    main()
