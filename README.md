# STM32F103C8T6 Dev-Beta

基于 STM32F103C8T6 + 1.8寸 TFT-LCD (ST7735S) 的嵌入式开发调试平台。

## 硬件

| 模块 | 型号/参数 |
|------|----------|
| MCU | STM32F103C8T6 (Cortex-M3, 72MHz, 64KB Flash, 20KB SRAM) |
| 库 | STM32CubeF1 HAL v1.8.6 |
| 显示屏 | 1.8寸 TFT, ST7735S, 128×160, 65K色 |
| 接口 | 硬件 SPI1 (18MHz) + DMA1_Ch3, USART1 (115200bps) |
| 调试 | SWD (ST-LINK), USB-UART (COM32) |
| LED | PC13 心跳灯 |

## 引脚

| 引脚 | 功能 |
|------|------|
| PA0 | TFT BLK |
| PA1 | TFT CS |
| PA2 | TFT RST |
| PA3 | TFT DC |
| PA5 | SPI1 SCK |
| PA7 | SPI1 MOSI |
| PA9 | USART1 TX |
| PA10 | USART1 RX |
| PC13 | LED |

## 工程结构

```
├── CMSIS/          # Cortex-M3 CMSIS
├── Drivers/        # MCU 外设 (SPI/DMA/USART + HAL)
├── Hardware/       # 器件 (LCD/LED)
├── User/           # 应用 (main/bsp_init)
├── Startup/        # 启动文件
├── MDK-ARM/        # Keil5 工程
└── Doc/            # 文档
```

## 编译烧录

```bash
# 编译
build.bat

# 烧录
STM32_Programmer_CLI.exe -c port=SWD -d MDK-ARM\Output\DevBeta_STM32F103.hex -v --start
```

## 版本

当前: **v0.0.1-beta** | 详见 [CHANGELOG.md](CHANGELOG.md)
