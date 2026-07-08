# 固件版本更新日志 (Changelog)

**项目名称**: STM32F103C8T6 Dev-Beta 嵌入式开发调试平台
**硬件平台**: STM32F103C8T6 (Cortex-M3, 72MHz, 64KB Flash, 20KB SRAM)
**MCU 库**: STM32CubeF1 HAL v1.8.6
**显示器**: 1.8寸 TFT-LCD (ST7735S, 128×160, 硬件SPI+DMA)
**当前固件版本**: v0.0.1-beta

---

## 📐 目录结构

```
Dev-beta-STM32F103/
├── CMSIS/                     # Cortex-M3 CMSIS + 器件头文件
├── Drivers/                   # MCU 外设驱动
│   ├── STM32F1xx_HAL_Driver/  #   HAL 库 (Inc + Src)
│   ├── USART/usart.c+h        #   USART1 PA9/PA10 (HAL UART)
│   ├── SPI/spi.c+h            #   SPI1 PA5/PA7 (寄存器级, 18MHz)
│   └── DMA/dma.c+h            #   DMA1_Ch3 (寄存器级, SPI_Tx)
├── Hardware/                  # 外部器件驱动
│   ├── LCD/lcd.c+h            #   ST7735S 128×160 (SPI+DMA)
│   └── LED/led.c+h            #   PC13 心跳灯 (HAL GPIO)
├── Startup/                   # 启动文件 (startup_stm32f103xb.s)
├── User/                      # 应用层
│   ├── main.c+h               #   入口 + Demo
│   ├── bsp_init.c+h           #   集中初始化 (时钟/LED/USART/LCD)
│   └── stm32f1xx_hal_conf.h   #   HAL 模块配置
├── MDK-ARM/                   # Keil5 工程 + 编译产物
├── Doc/                       # 文档
├── CHANGELOG.md
├── README.md
├── keil_config.json
└── .gitignore
```

## 🔧 引脚分配总表

| 引脚 | 功能 | 方向 | 说明 |
|------|------|------|------|
| PA0 | BLK | 推挽输出 | TFT 背光 (高电平亮) |
| PA1 | CS | 推挽输出 | TFT 片选 (低有效) |
| PA2 | RST | 推挽输出 | TFT 硬件复位 |
| PA3 | DC | 推挽输出 | TFT 数据/命令 |
| PA5 | SCK | AF推挽 | SPI1 时钟 (18MHz) |
| PA7 | MOSI | AF推挽 | SPI1 数据 |
| PA9 | USART1_TX | AF推挽 | 串口发送 (COM32, 115200) |
| PA10 | USART1_RX | 浮空输入 | 串口接收 |
| PC13 | LED | 推挽输出 | 心跳灯 (低电平亮) |
| PA13 | SWDIO | SWD | ST-LINK |
| PA14 | SWCLK | SWD | ST-LINK |

---

## 📄 固件版本演进变更详情

* **v0.0.1** (2026-06-08)
  * 🏗️ 工程地基 — STM32CubeF1 HAL 全新工程
    - CMSIS + STM32F1xx_HAL_Driver (STM32CubeF1 v1.8.6)
    - 启动文件: startup_stm32f103xb.s, system_stm32f1xx.c
    - Keil MDK-ARM V5 (ARMCC V5.06), 自动化烧录 (STM32CubeProgrammer)
  * ⚡ 系统时钟: HSE 8MHz → PLL ×9 → 72MHz (HAL RCC)
  * 📡 USART1: HAL UART, PA9/PA10, 115200bps, uart_puts/putc/puts_hex
  * 💡 LED: PC13 HAL GPIO, Toggle + SysTick HAL_Delay 1Hz
  * 🖥️ TFT ST7735S 驱动
    - 硬件 SPI1 (CPOL=High, CPHA=2Edge, 18MHz, 寄存器直配)
    - DMA1_Ch3 像素批量填充 (寄存器级, 轮询TC标志)
    - lcdfont.h 字库: 12/16/24/32px 四套 ASCII 字体
    - LCD_Fill / DrawPoint / DrawLine / DrawRect / DrawCircle
    - LCD_ShowChar / ShowString (8×16 ASCII)
  * 🧱 模块化架构
    - Drivers (外设) / Hardware (器件) / User (应用) 三层分离
    - bsp_init 集中初始化 (时钟 + LED + USART + LCD)
    - 编译: 0 Error 0 Warning, Flash 18.4KB
  * 🔧 开发环境
    - COM32, ST-LINK SWD
    - STM32CubeProgrammer --start 自动启动
    - SysTick_Handler → HAL_IncTick → HAL_Delay 正常

---

## 🔜 计划中 (Roadmap)

* **v0.0.2** — SPI 改为 HAL SPI API, DMA 中断模式
* **v0.0.3** — 中文字库 + 串口交互终端
