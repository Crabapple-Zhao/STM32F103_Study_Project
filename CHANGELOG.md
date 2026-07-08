# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

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
