# STM32F103C8T6 Dev-Beta

基于 STM32F103C8T6 + 1.8寸 TFT-LCD (ST7735S) 的嵌入式开发调试平台，集成 **GuiLite v3.4** 超轻量 GUI 渲染引擎。

## 硬件

| 模块 | 型号/参数 |
|------|----------|
| MCU | STM32F103C8T6 (Cortex-M3, 72MHz, 64KB Flash, 20KB SRAM) |
| 库 | STM32CubeF1 HAL v1.8.6 (ARMCC V5.06) |
| 显示屏 | 1.8寸 TFT, ST7735S, 128x160, RGB565, SPI+DMA |
| GUI | GuiLite v3.4 最小渲染子集 (~5KB RAM) |
| 接口 | 硬件 SPI1 (18MHz) + DMA1_Ch3, USART1 (115200bps) |
| 调试 | SWD (ST-LINK), USB-UART (COM20) |
| LED | PA11 心跳灯 (低电平点亮) |
| 按键 | KEY1=PA0, KEY2=PC13 (高电平按下) |

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
| PA13/P14 | SWDIO/SWCLK (调试) |

## 工程结构

```
Dev-beta-STM32F103/
├── CMSIS/                    # Cortex-M3 CMSIS + 器件头文件
├── Drivers/
│   ├── STM32F1xx_HAL_Driver/ # HAL 库 (Inc + Src)
│   ├── USART/usart.c+h       # USART1 驱动
│   ├── SPI/spi.c+h           # SPI1 寄存器级驱动 (18MHz)
│   └── DMA/dma.c+h           # DMA1_Ch3 寄存器级驱动 (SPI_Tx)
├── Hardware/
│   ├── LCD/lcd.c+h           # ST7735S 128x160 (SPI+DMA)
│   ├── LED/led.c+h           # PA11 心跳灯
│   └── KEY/key.c+h           # KEY1(PA0) + KEY2(PC13) 按键驱动
├── GuiLite/                   # GuiLite v3.4 移植层
│   ├── GuiLite_min.h         # 最小渲染子集 (排除控件系统)
│   ├── GuiLiteAdapter.cpp    # LCD 驱动适配桥接
│   ├── gfx_types.h           # EXTERNAL_GFX_OP 类型定义
│   └── font_ascii_8x16.h     # 8x16 ASCII 字体 (ROW-MAJOR 1bpp)
├── User/
│   ├── main.cpp              # C++ 入口 / 按键测试 Demo
│   ├── main.h                # 主头文件
│   ├── bsp_init.c+h          # 板级初始化 + 中断处理函数
├── Startup/startup_stm32f103xb.s  # 启动文件 (Stack=1KB, Heap=4KB)
├── MDK-ARM/Project.uvprojx   # Keil MDK-ARM V5 工程 (C++ 模式)
└── CHANGELOG.md              # 变更记录
```

## 编译烧录

```bash
# Keil 命令行编译 (需安装 ARMCC V5)
UV4.exe -b MDK-ARM\Project.uvprojx -j0 -o build_log.txt

# STM32CubeProgrammer 烧录
STM32_Programmer_CLI.exe -c port=SWD -d MDK-ARM\Output\DevBeta_STM32F103.hex -rst
```

**编译资源占用**: Code ~8KB, RO-data ~2KB, RW-data ~40B, ZI-data ~6.5KB (总计 ~10.5KB Flash / ~6.5KB SRAM)

## 架构说明

```
main.cpp (C++)
  └─> c_display → c_surface → EXTERNAL_GFX_OP 回调
        └─> GuiLiteAdapter.cpp (C++)
             └─> lcd.c (C) → SPI/DMA 硬件驱动
```

- **中断处理函数** (`SysTick_Handler` / `DMA1_Channel3_IRQHandler`) 定义在 `bsp_init.c` 中，
  避免 C++ 名称修饰导致链接器移除
- **GuiLite_min.h** 从完整 GuiLite.h 提取最小子集，排除控件/消息系统的静态变量（节省 ~5KB RAM）
- **draw_lattice** 已适配字体数据为 ROW-MAJOR 1bpp 格式（每字节=1行8像素，MSB=左像素）
- **按键** KEY1/KEY2 配置为下拉输入，高电平按下；触发电平宏定义在 `key.h`/`led.h` 中

## 版本

当前: **v0.2.1** | 详见 [CHANGELOG.md](CHANGELOG.md)