# 任务交接文档 — UI 框架开发

> **生成时间**: 2025-07-07  
> **当前版本**: v0.2.2 (未提交)  
> **工作区路径**: `F:\Agent_Workspace\TRAE\Dev-beta-STM32F103`

---

## 1. 当前项目状态

### 1.1 版本与 Git 状态

| 项目 | 状态 |
|------|------|
| 当前版本 | v0.2.2 (代码中 APP_VERSION 仍为 v0.2.1，需升级) |
| 最新 tag | v0.2.1 (bcaa7f1) |
| 未提交文件 | `MDK-ARM/Project.uvprojx`, `User/bsp_init.c`, `User/main.cpp` (modified) |
| 未跟踪文件 | `Drivers/Timer/`, `Hardware/Encoder/` (new) |

### 1.2 编译烧录

```bash
# 编译 (Keil ARMCC V5)
UV4.exe -b MDK-ARM\Project.uvprojx -j0 -o build_log.txt

# 烧录 (SWD)
STM32_Programmer_CLI.exe -c port=SWD -d MDK-ARM\Output\DevBeta_STM32F103.hex -rst
```

烧录方法：**SWD** (ST-LINK)，不使用 ISP（ISP 需要手动切换 BOOT0 跳线，太麻烦）。

---

## 2. 完整引脚映射

| 引脚 | 功能 | 说明 |
|------|------|------|
| PA0 | KEY1 | 按键，下拉输入，高电平按下 |
| PA1 | TFT CS | 片选 |
| PA2 | TFT RST | 复位 |
| PA3 | TFT DC | 数据/命令 |
| PA4 | TFT BLK | 背光控制 |
| PA5 | SPI1 SCK | 时钟 |
| PA7 | SPI1 MOSI | 数据 |
| PA9 | USART1 TX | 串口发送 |
| PA10 | USART1 RX | 串口接收 |
| PA11 | LED | 心跳灯，低电平点亮 |
| PA13 | SWDIO | 调试 |
| PA14 | SWCLK | 调试 |
| PC13 | KEY2 | 按键，下拉输入，高电平按下 |
| PB5 | ENC SW | 编码器按键，上拉输入，低电平按下 |
| PB6 | ENC A | TIM4_CH1，编码器 A 相 |
| PB7 | ENC B | TIM4_CH2，编码器 B 相 |

---

## 3. 当前屏幕布局 (128x160)

| 行 (y) | 内容 | 说明 |
|--------|------|------|
| 10 | "STM32F103 Dev-Beta" | 标题 |
| 28 | "SPI+DMA OK" | 硬件状态 |
| 46 | "USART1 OK" | 硬件状态 |
| 62 | "v0.2.1" | 版本号 (需更新为 v0.2.2) |
| 80 | dividing line | 分隔线 |
| 98 | "Hello, GuiLite!" | 测试文字 |
| 116 | `ENC:+00000  SW:OFF` | 编码器显示 (y=116) |
| 134 | `KEY1  KEY2` | 按键状态 (y=134) |

---

## 4. 已完成的编码器模块

### 4.1 文件结构

```
Drivers/Timer/timer.c+h    — TIM4 编码器模式 (寄存器级, TI1+TI2 双沿四倍频)
Hardware/Encoder/encoder.c+h — 编码器封装 (Encoder_Init/GetCount/ResetCount/SW_Read)
```

### 4.2 API

```c
void    Encoder_Init(void);        // 初始化 TIM4 编码器模式 + PB5 上拉输入
int16_t Encoder_GetCount(void);    // 读取当前计数值 (有符号)
void    Encoder_ResetCount(void);  // 计数器归零
uint8_t Encoder_SW_Read(void);     // SW 按键状态 (1=按下, 0=释放)
```

### 4.3 主循环逻辑 (main.cpp)

当前主循环采用**局部刷新**策略：
- 独立变量跟踪 ENC 计数、SW 状态、KEY1、KEY2 的上次值
- 每 50ms 轮询一次
- 只有状态变化时才重绘对应区域
- `draw_string` 使用不透明黑色背景 `GL_RGB(0,0,0)` 一步完成清屏+绘制

---

## 5. 下一步任务：UI 框架

### 5.1 需求概述

开发一个编码器驱动的菜单 UI 框架：

1. **首页** — 多个选项 (如"选项A"、"选项B"等) + 信息显示区域
2. **编码器导航** — 旋转编码器在选项间移动光标/高亮
3. **按下确认** — 按下编码器进入选中选项的二级界面
4. **长按返回** — 长按编码器 SW 返回上一级界面
5. **多级界面** — 支持至少两级菜单深度

### 5.2 技术选型建议

- **GuiLite 控件**: 完整版 `GuiLite.h` 有 `c_list_box` 控件 (MAX_ITEM_NUM=4, ITEM_HEIGHT=45px)，当前使用 `GuiLite_min.h` 精简版不含控件类。如需使用需切换到头文件 `GuiLite.h`
- **或者**: 基于现有的 `c_surface` + `draw_string` 手工实现菜单，灵活性更高
- **编码器长按检测**: 需要记录 SW 按下持续时间的计时逻辑

### 5.3 待办事项

- [ ] 将 `APP_VERSION` 从 `"v0.2.1"` 更新为 `"v0.2.2"`
- [ ] 提交当前未提交的编码器改动 (`git commit`)
- [ ] 设计首页 UI 布局 (选项列表 + 状态栏)
- [ ] 实现编码器菜单导航逻辑 (旋转移动光标，按下确认，长按返回)
- [ ] 实现二级界面框架
- [ ] 测试多级界面切换流程

---

## 6. 重要经验教训

### 6.1 屏幕显示

- **屏幕尺寸**: 128x160 像素，所有 UI 元素必须在范围内
- **防闪烁**: 使用 `draw_string` 不透明背景 (`GL_RGB(0,0,0)`) 替代 `fill_rect` + 透明背景两步操作
- **局部刷新**: 只更新状态变化的区域，避免全屏重绘
- **字体**: 8x16 ASCII 字体，每个字符占 8px 宽

### 6.2 DMA/SPI

- SPI 数据帧格式必须设为 16-bit (`SPI_CR1_DFF`) 再进行 DMA 传输
- DMA 源地址必须使用静态变量 (BSS 段)，不能用栈变量
- 禁止使用 `DMA_CCR_MINC` (内存增量) 和 `DMA_CCR_CIRC` (循环模式)，会导致花屏

### 6.3 版本管理

- 版本号在 `User/main.cpp` 的 `APP_VERSION` 宏中定义
- 串口输出和屏幕显示都引用此宏
- Commit message 格式: `vX.Y.Z: type(scope) 描述`
- 编译后需同步更新 `README.md` 和 `CHANGELOG.md`

---

## 7. 文件清单

### 7.1 核心文件 (按重要性)

| 文件 | 作用 |
|------|------|
| `User/main.cpp` | 主程序入口，UI 逻辑，APP_VERSION 宏 |
| `User/bsp_init.c` | 板级初始化 (BSP_Init)，中断处理函数 |
| `MDK-ARM/Project.uvprojx` | Keil 工程文件 |
| `Hardware/LCD/lcd.c+h` | ST7735S LCD 驱动 |
| `GuiLite/GuiLiteAdapter.cpp` | GuiLite ↔ LCD 桥接 |
| `GuiLite/GuiLite_min.h` | GuiLite 最小渲染子集 |
| `Hardware/Encoder/encoder.c+h` | 编码器驱动 |
| `Drivers/Timer/timer.c+h` | TIM4 编码器模式 |
| `Hardware/KEY/key.c+h` | 按键驱动 |
| `Hardware/LED/led.c+h` | LED 驱动 |
| `README.md` | 项目文档 (已更新 v0.2.2) |
| `CHANGELOG.md` | 变更记录 (已更新 v0.2.2) |

---

*此文档为临时交接文档，任务完成后可删除。*