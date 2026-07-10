# STM32F103C8T6 Dev-Beta

基于 STM32F103C8T6 + 1.8寸 TFT-LCD (ST7735S) 的嵌入式开发调试平台，集成 **Astra UI** 轻量级菜单框架（磁贴页/列表页/摄像机系统/缓动动画）。

## 硬件

| 模块 | 型号/参数 |
|------|----------|
| MCU | STM32F103C8T6 (Cortex-M3, 72MHz, 64KB Flash, 20KB SRAM) |
| 库 | STM32CubeF1 HAL v1.8.6 (ARMCC V5.06) |
| 显示屏 | 1.8寸 TFT, ST7735S, 128x160, RGB565, SPI+DMA |
| GUI | Astra UI 轻量级菜单框架 (1bpp 虚拟显存 ~2.5KB RAM) |
| 接口 | 硬件 SPI1 (18MHz) + DMA1_Ch3, USART1 (115200bps) |
| 调试 | SWD (ST-LINK), USB-UART (COM9) |
| LED | PA11 心跳灯 (低电平点亮) |
| 按键 | KEY1=PA0, KEY2=PC13 (高电平按下) |
| 编码器 | A=PB6, B=PB7, SW=PB5 (TIM4 编码器模式) |

## 引脚

| 引脚 | 功能 |
|------|------|
| PA0 | KEY1 (按键，下拉输入) |
| PA1 | TFT CS (片选) |
| PA2 | TFT RST (复位) |
| PA3 | TFT DC (数据/命令) |
| PA4 | TFT BLK (背光) |
| PA5 | SPI1 SCK |
| PA7 | SPI1 MOSI |
| PA9 | USART1 TX |
| PA10 | USART1 RX |
| PA11 | LED (心跳灯，低电平点亮) |
| PC13 | KEY2 (按键，下拉输入) |
| PB5 | ENC SW (编码器按键，上拉输入，低电平按下) |
| PB6 | ENC A (TIM4_CH1，编码器 A 相) |
| PB7 | ENC B (TIM4_CH2，编码器 B 相) |
| PA13/P14 | SWDIO/SWCLK (调试) |

## 工程结构

```
Dev-beta-STM32F103/
├── CMSIS/                    # Cortex-M3 CMSIS + 器件头文件
├── Drivers/
│   ├── STM32F1xx_HAL_Driver/ # HAL 库 (Inc + Src)
│   ├── USART/usart.c+h       # USART1 驱动
│   ├── SPI/spi.c+h           # SPI1 寄存器级驱动 (18MHz)
│   ├── DMA/dma.c+h           # DMA1_Ch3 寄存器级驱动 (SPI_Tx)
│   └── Timer/timer.c+h       # TIM4 编码器模式驱动 (寄存器级)
├── Hardware/
│   ├── LCD/lcd.c+h           # ST7735S 128x160 (SPI+DMA)
│   ├── LED/led.c+h           # PA11 心跳灯
│   ├── KEY/key.c+h           # KEY1(PA0) + KEY2(PC13) 按键驱动
│   └── Encoder/encoder.c+h   # 编码器旋钮驱动 (A/B/SW)
├── Astra/                     # Astra UI 框架
│   ├── hal/                  # HAL 抽象层
│   │   ├── hal.h/cpp         # HAL 基类 + 默认实现
│   │   └── hal_port.h/cpp    # STM32F103 移植层 (1bpp→RGB565 桥接 + 编码器输入)
│   └── astra/
│       ├── config/config.h   # UI 配置 (128x160 适配 + 8x16 字体)
│       ├── ui/
│       │   ├── launcher.h/cpp # 调度器 (页面切换/动画/摄像机)
│       │   └── element/page/
│       │       └── item.h/cpp # 菜单/选择器/摄像机类
│       ├── astra_icons.h      # 菜单磁贴图标数据 (home/gear/info/tool, 32x32 1bpp)
│       └── astra_rocket.h/cpp # 启动入口 (开机画面 + 菜单树定义 + astraLoop)
├── User/
│   ├── main.cpp              # C++ 入口 (Astra UI 主循环)
│   ├── main.h                # 主头文件
│   ├── app_config.h          # 固件版本/目标硬件/串口等项目级常量
│   ├── app_log.h             # 轻量级串口日志宏
│   ├── bsp_init.c+h          # 板级初始化 + 中断处理函数
├── Startup/startup_stm32f103xb.s  # 启动文件 (Stack=1KB, Heap=4KB)
├── MDK-ARM/Project.uvprojx   # Keil MDK-ARM V5 工程 (C++ 模式, --gnu --cpp11)
└── CHANGELOG.md              # 变更记录
```

## 编码器

- **硬件连接**: A=PB6(TIM4_CH1), B=PB7(TIM4_CH2), SW=PB5, C=GND
- **驱动方式**: TIM4 编码器模式 (TI1+TI2 双沿四倍频)，寄存器级配置
- **SW 按键**: 上拉输入，按下时接通 GND (低电平有效)
- **API**: `Encoder_Init()` / `Encoder_GetCount()` / `Encoder_ResetCount()` / `Encoder_SW_Read()`
- **UI 导航**: 旋转=上下导航，SW 短按=进入/确认，SW 长按=返回上一级

## 编译烧录

```bash
# Keil 命令行编译 (需安装 ARMCC V5)
UV4.exe -b MDK-ARM\Project.uvprojx -j0 -o build_log.txt

# STM32CubeProgrammer SWD 烧录
STM32_Programmer_CLI.exe -c port=SWD -d MDK-ARM\Output\DevBeta_STM32F103.hex -rst
```

**编译资源占用**: Code 30796B, RO-data 4352B, RW-data 268B, ZI-data 17900B (Keil ARMCC V5.06, MicroLib)

## 架构说明

```
main.cpp (C++)
  └─> astraCoreInit() / astraLoop()  (Astra UI 框架)
        └─> hal_port.cpp (1bpp 虚拟显存 → RGB565 桥接)
             └─> lcd.c (C) → LCD_WriteLine() → SPI/DMA 硬件驱动
```

- **Astra UI 框架** 原为 1bpp 单色 OLED 设计，移植层使用 2560 字节 1bpp 虚拟显存，
  canvasUpdate 时逐行转换为 RGB565 并通过 SPI+DMA 推送到 LCD
- **HAL 抽象层** (`hal.h/cpp`) 定义绘图 API，`hal_port.cpp` 提供具体实现
- **项目级配置** (`app_config.h`) 统一固件版本、目标硬件、显示屏名称和 USART 调试参数，避免多处字符串不一致
- **菜单系统** 支持磁贴页 (TILE) 和列表页 (LIST) 两种风格，摄像机系统实现页面切换动画
- **编码器输入** 旋转=上下导航，SW 短按=进入/确认，SW 长按=返回上一级
- **中断处理函数** (`SysTick_Handler` / `DMA1_Channel3_IRQHandler`) 定义在 `bsp_init.c` 中，
  避免 C++ 名称修饰导致链接器移除
- **ARMCC V5.06 兼容性** 使用 `--gnu --cpp11` 编译选项，源码已适配 ARMCC V5.06 有限的 C++11 支持

## 版本

当前: **v0.3.9** | 详见 [CHANGELOG.md](CHANGELOG.md)
