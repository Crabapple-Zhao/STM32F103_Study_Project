# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [0.2.2] - 2025-07-07

### Added
- `Drivers/Timer/timer.c+h` — TIM4 编码器模式驱动 (TI1+TI2 双沿四倍频，寄存器级配置)
- `Hardware/Encoder/encoder.c+h` — 编码器旋钮驱动模块 (A=PB6, B=PB7, SW=PB5)
- 编码器 Demo：屏幕第 8 行显示 ENC 数值+SW 状态，第 9 行保留按键测试
- 编码器 API：`Encoder_Init()` / `Encoder_GetCount()` / `Encoder_ResetCount()` / `Encoder_SW_Read()`

### Changed
- 按键测试区域缩小并下移，为编码器显示腾出空间 (ENC_DISP_Y=116, KEY_DISP_Y=134)
- 屏幕刷新优化为局部刷新：独立判断每个区域状态变化，只重绘变化区域

### Fixed
- **SW:OFF 出界**：SW 区域右移 (x=76→100)，确保在 128px 屏幕内
- **编码器数值叠加**：draw_string 改用不透明黑背景 `GL_RGB(0,0,0)` 替代透明背景，一步覆盖旧内容
- **整行闪烁**：去除 fill_rect 清屏步骤，draw_string 用不透明背景一步完成

---

## [0.2.1] - 2025-07-06

### Added
- `Hardware/KEY/key.c+h` — 按键驱动模块，支持 KEY1(PA0) 和 KEY2(PC13)，下拉输入
- 按键测试 Demo：屏幕实时显示按键状态（按下绿色/蓝色，松开灰色）
- 触发电平宏抽象：`LED_ACTIVE_LEVEL`(led.h) 和 `KEY_ACTIVE_LEVEL`(key.h)

### Changed
- LED 心跳灯从 PC13 移至 PA11
- TFT BLK 背光从 PA0 移至 PA4（解决与 KEY1 引脚冲突）
- 按键初始化 `KEY_Init()` 放在 `LCD_Init()` 之后（避免 PA0 被覆盖）
- 屏幕刷新优化：仅在按键状态变化时更新显示区域

### Fixed
- **PA0 引脚冲突**：KEY1 与 BLK 共用 PA0，按下按键时背光变亮
- **文字遮挡**：按键状态区域与提示文字重叠，调整布局到 y=132

---

## [0.2.0] - 2025-06-14

### Added
- GuiLite v3.4 超轻量 GUI 渲染引擎移植到 STM32F103C8T6
- 8x16 ASCII 字体渲染支持（行优先 1bpp 格式）
- 无帧缓冲显示模式，通过 EXTERNAL_GFX_OP 回调直接写 LCD
- `GuiLite/GuiLite_min.h` — 最小化渲染子集（排除控件/消息系统，节省 ~5KB RAM）
- `GuiLite/GuiLiteAdapter.cpp` — GuiLite 与 LCD 驱动的桥接层
- `GuiLite/gfx_types.h` — EXTERNAL_GFX_OP 共享类型定义（消除 ODR 违规）
- `GuiLite/font_ascii_8x16.h` — 8x16 ASCII 字体点阵数据
- `User/main.cpp` — C++ 入口文件及 GuiLite 测试 UI
- Keil 工程切换 C++ 编译模式（FileType=8）

### Fixed
- **C++ 名称修饰导致程序崩溃**：中断处理函数(SysTick_Handler / DMA1_Channel3_IRQHandler)从 main.cpp 移入 bsp_init.c(C 文件)，解决 ARMCC V5 的 extern "C" 无效导致链接器移除中断向量的问题
- **字体渲染乱码**：重写 draw_lattice 函数，从 GuiLite 原始的列优先灰度格式改为行优先 1bpp 格式，匹配 font_ascii_8x16.h 的实际数据格式
- **DMA 竞态条件**：修复 DMA_SPI1_Tx / DMA_SPI1_Tx16 在通道使能前未清零 CCR 寄存器的问题
- **颜色转换错误**：修正 GL_RGB_32_to_16 宏的 ARGB(32bit) -> RGB565(16bit) 提取逻辑

### Changed
- Heap_Size 从 0x200 (512B) 增至 0x1000 (4KB)，适配 GuiLite 动态分配需求
- 所有 C 头文件添加 extern "C" 保护（lcd.h / led.h / usart.h / bsp_init.h）
- DMA 驱动清理调试输出，统一超时处理逻辑
- 移除无效工程配置：uAC6=0, GUILITE_ON 宏定义

### Resource Usage (vs v0.1.0)
| 指标 | v0.1.0 | v0.2.0 | 变化 |
|------|--------|--------|------|
| Flash (Code+RO) | ~8KB | ~9.9KB | +1.9KB |
| RAM (RW+ZI) | ~2KB | ~6.5KB | +4.5KB |

---

## [0.1.0] - 2025-06-13

### Added
- STM32F103C8T6 初始工程框架（HAL 库 v1.8.6）
- ST7735S TFT LCD 驱动（128x160, SPI+DMA）
- USART1 串口驱动（PA9/PA10, 115200bps）
- PC13 LED 驱动
- Keil MDK-ARM V5 工程配置