# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [0.3.0] - 2026-07-07

### Added
- **Astra UI 框架移植** — 从 `astra-ui-stm32` 移植轻量级菜单 UI 框架到 1.8寸 ST7735S 彩色屏
- `Astra/hal/hal.h/cpp` — HAL 抽象基类 + 默认实现
- `Astra/hal/hal_port.h/cpp` — STM32F103 核心移植层：1bpp 虚拟显存 (2560B) → RGB565 逐行转换 → SPI+DMA 推送
- `Astra/astra/config/config.h` — UI 配置适配 128x160 屏幕 + 8x16 字体参数
- `Astra/astra/ui/launcher.h/cpp` — 调度器（页面切换/动画/摄像机系统）
- `Astra/astra/ui/element/page/item.h/cpp` — 菜单/选择器/摄像机类
- `Astra/astra/astra_logo.h/cpp` — Logo 启动动画
- `Astra/astra/astra_rocket.h/cpp` — 启动入口：菜单树定义（Home/Settings/About/Tools + 子菜单）+ 非阻塞 `astraLoop()` 接口
- `Hardware/LCD/lcd.c` 新增 `LCD_WriteLine()` / `LCD_WriteBegin()` / `LCD_StreamLine()` / `LCD_WriteEnd()` — 逐行 RGB565 写入接口，供 hal_port 桥接调用
- 编码器替代按键作为 UI 导航：旋转=上下导航，SW 短按=进入/确认，SW 长按=返回
- 串口启动输出版本号 `v0.3.0`，主循环输出 FPS 帧率
- 菜单树：Home(-Status/-Uptime/-Memory)、Settings(-Brightness/-Contrast/-Reset)、About(-Astra UI/-STM32F103/-ST7735S)、Tools(-Encoder/-LCD Test/-LED Blink/-SPI DMA/-Key Scan)

### Changed
- `User/main.cpp` 重写为 Astra UI 主循环（`astraCoreInit()` + `astraLoop()`）
- Keil 工程编译选项改为 `--cpp11`，Astra 组替换原 GuiLite 组
- **启用 MicroLib**（`<useUlib>1</useUlib>`）— 消除标准 C 库 semihosting `BKPT #0xAB` 导致的 HardFault
- `Startup/startup_stm32f103xb.s` Heap_Size 从 0x2000 (8KB) 增至 0x3000 (12KB)，适配 19 个 Menu 对象的动态分配
- `Hardware/LCD/lcdfont.h` 中文字模部分用 `#if 0` 排除（`--cpp11` 下 GBK 字符串类型检查冲突，且 lcd.c 未使用）

### Fixed
- **Semihosting HardFault** — 标准 C 库（非 MicroLib）使用 semihosting `BKPT #0xAB` 进行 I/O，无调试器半主机支持时触发 HardFault。通过 STM32CubeProgrammer `-hf` HardFault 分析器 + `-r32fast` 异常栈帧读取定位故障指令，启用 MicroLib 解决
- **drawLogo 卡死** — `drawLogo()` 的浮点动画 `yBackGround == 0 - screenHeight - 1` 精确比较永不满足 + 每帧全屏 drawBox 浮点运算过重，暂跳过启动动画
- **exitAnimation 越界写内存** — `item.h` 中 `bufferLen` 为 `uint8_t` 导致 2560 > 255 溢出，open()/deInit() 时越界写入 17920 字节破坏堆栈引发 BusFault → 改为 `uint32_t`
- **图标显示乱线条** — `_drawBMP` 格式不匹配，数据是行优先 LSB first（标准 XBM），修正为 `byteIdx = y*((w+7)/8) + x/8, bit = 1<<(x%8)`
- **点击磁贴卡死** — `popInfo()` 浮点精确比较永不满足导致阻塞循环；open() 失败直接返回不调用 popInfo
- **编码器过灵敏** — TIM4 四倍频模式每转一格产生 4 个脉冲，之前每个脉冲都触发移动 → 引入累加器阈值 4 才触发一次
- **ARMCC V5.06 C++11 兼容性** — 逐一修复 ARMCC V5.06 有限 C++11 支持导致的编译错误：
  - `std::move` 不支持 → 直接传值 (`hal.h`, `item.cpp`)
  - `[[nodiscard]]` 不支持 → 移除属性 (`item.h`)
  - `std::vector::data()` 不支持 → `empty() ? nullptr : &v[0]` (`item.cpp`)
  - `std::vector` initializer list 构造不支持 → 改用 `push_back` 或数组+范围构造 (`item.cpp`, `astra_rocket.cpp`)
  - `<cstdlib>` 不引入 `srand/rand` 到全局 → 改用 `<stdlib.h>` (`astra_logo.cpp`)
  - `<cstring>` 不引入 `memset` 到全局 → 添加 `<string.h>` (`hal.cpp`, `hal_port.cpp`)
  - `ceil`/`floor` 未定义 → 改用 `(int)(... + 0.5f)` / `(int)(...)` (`item.cpp`)
  - 类内成员初始化不支持 → 添加 `--cpp11` 选项 (`config.h`)
- `item.cpp` `Selector::destroy()` 补充缺失的 `return true`

### Removed
- `GuiLite/` 目录 — GuiLite v3.4 渲染引擎（已被 Astra UI 替代）

### Resource Usage (vs v0.2.2)
| 指标 | v0.2.2 | v0.3.0 | 变化 |
|------|--------|--------|------|
| Flash (Code+RO) | ~10.5KB | ~33KB | +22.5KB |
| RAM (RW+ZI) | ~6.5KB | ~18KB | +11.5KB |
| 运行帧率 | — | ~23 FPS | — |

---

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