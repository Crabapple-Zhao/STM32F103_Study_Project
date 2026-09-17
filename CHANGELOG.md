# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [0.4.7] - 2026-09-17

### Changed
- 顶部状态栏图标从 HAL 移植层拆分，复用 Bitmap 资源描述；保留原电池/Wi-Fi/TF 卡点阵与默认排列。
- 新增显隐、替换图案和恢复默认接口；布局预留标题空间，对无效资源、超高与空间不足的图标跳过绘制。
- 新增状态栏 JSON 素材源及 `icon_gen.py --status` 工具，自动生成点阵、尺寸、默认排列和预览；无需修改 HAL 即可增删、更换图标。
- Bitmap 校验增加零宽、零高检查。

### Verified
- 素材迁移逐字节一致；Python 工具测试和 C++ 布局/接口测试通过。
- Keil 全量编译通过，0 Error(s)、0 Warning(s)；Code 43988B、RO 5384B、RW 336B、ZI 17752B。
- ST-LINK 烧录和读回校验成功；SWD 读取运行计数递增、FPS=20，实际显存状态栏区域与原版逐像素一致。
- 用户接回串口后，COM4 / 115200 硬复位日志确认 `version=v0.4.7`、`root_items=4`、Astra 初始化成功；监听期间 FPS=20，无异常复位或错误日志。
- 用户已复核实屏画面，确认无问题。详见 `Doc/状态栏图标维护.md`。

## [0.4.6] - 2026-09-11

### Added
- 新增 `Drivers/PWM/pwm.c/h`：TIM3_CH4 @ PB1 PWM 输出驱动，预设 100Hz/1kHz/10kHz/100kHz 四档频率（PSC=71 固定、ARR 随档位变化），占空比 0~100%。
- 新增 `Astra/astra/pages/pwm_page.cpp/h`：PWM 输出子菜单，`Control` 页面（旋钮调占空比、短按开关输出、实时占空比/电压/进度条）与 `Frequency` 页面（旋钮切换频率档位）。
- 新增 `Menu::ContentKeyHandler` 内容页按键回调，`Launcher` 将内容页按键事件（旋转/短按/长按）先交给页面处理，未消费才走默认导航（长按返回全局默认不变）。

### Changed
- 主菜单第三个磁贴由 About 改为 Output，删除原 About 信息项（-Astra UI/-STM32F103/-ST7735S），挂接 PWM-Output 子菜单。
- 图标资源新增 `icon_output`（复用齿轮点阵），删除 `icon_about`。

### Removed
- 删除 About 磁贴遗留的信息图标 `pic_info_data` 与 `bmp_info`，同步清理 `Tools/icons/icons.json` 的 about 素材与 `Tools/icon_gen.py` 的 about 符号映射。

### Fixed
- 修复 PB1 无 PWM 输出的问题：GPIO 配置值 `0x2`（普通推挽输出）误写为 `0xA`（复用推挽输出），导致定时器无法驱动 PB1、始终输出低电平。

### Verified
- Keil ARMCC V5.06 全量编译通过，0 Error(s)、0 Warning(s)；资源占用 Code 43680B、RO-data 5376B、RW-data 264B、ZI-data 17752B。
- `icon_gen.py` 校验 home/sensor/settings 资源与头文件逐字节一致，about 素材已移除。
- STM32CubeProgrammer SWD 下载和校验成功。

---

## [0.4.5] - 2026-09-11

### Added
- 新增 `Astra/astra/astra_icon.h` 图标资源接口：`Bitmap` 自带数据指针、宽高与字节数并提供 `valid()` 长度校验，`Icon` 组合普通图与可选选中图。
- 新增 Home、Sensor 原生 36x36 选中态图标，避免 30x30 小图非整数放大变形。
- 新增 `Tools/icons/icons.json` ASCII 点阵素材源与 `Tools/icon_gen.py` 图标工具（校验 / `--write` 生成 / `--preview` 预览），作为素材长期维护入口。

### Changed
- `Menu` 图标相关 5 个字段（`picData` / `selectedPicData` / 选中宽高 / `picSize`）合并为单个 `const Icon*`，新增磁贴只需绑定一个 Icon 对象。
- 磁贴渲染源尺寸改从 `Bitmap` 资源读取，与布局配置的显示尺寸解耦，防止修改全局尺寸后旧数组被错误解读；长度失配的资源跳过绘制，防止越界读取。
- `createTile()` 改为绑定 Icon 并增加启动时资源校验；`root_items` 改为运行时统计实际磁贴数量，增删入口无需同步修改日志。
- 重绘 Home（圆环）与 Sensor（芯片+波纹）图标，Settings 工具图标移除中心圆点。
- 调试串口重新枚举为 COM13，同步更新 `app_config.h`、README 与硬件接线表；README 修正启动文件栈堆描述为 Stack=2KB、Heap=12KB。

### Removed
- 删除无调用点的 `Menu(std::vector<uint8_t>)` 构造器与 `pic` 成员，消除点阵拷入 RAM 的潜在路径。
- 删除 `tmp/` 下 4 个临时图标脚本与预览图，由 `Tools/icon_gen.py` 取代。

### Verified
- Keil ARMCC V5.06 全量编译通过，0 Error(s)、0 Warning(s)；资源占用 Code 41392B、RO-data 5248B、RW-data 240B、ZI-data 17752B。
- `icon_gen.py` 校验 6 组资源 JSON 与头文件逐字节一致，`--write` 生成后解码回读自检通过。
- STM32CubeProgrammer SWD 下载和校验成功；COM13 硬复位日志确认 `version=v0.4.5`、`root_items=4`、无 `icon resource invalid` 报错，主循环 fps=20 稳定运行。
- 实机肉眼确认四个磁贴图标显示正常（含 Home/Sensor 36x36 选中态与 About/Settings 放大回退）。

---

## [0.4.4] - 2026-08-21

### Added
- 新增 `Application/Sensors` 应用层，使用静态 `SensorRuntime` 统一管理传感器连接、采样、重连、错误状态和页面退出释放流程。
- 新增 DHT11、BMP280、CS100A 应用适配器，保留各传感器原始状态码，同时向页面提供统一运行状态。
- 新增 `Astra/astra/pages/sensor_pages.cpp/.h`，集中管理传感器菜单注册、三级页面绘制和串口日志。
- 新增 `Hardware/DHT11/dht11_stm32f103.c/.h`，将 PA12、DWT 延时和临界区操作从 DHT11 协议核心中分离。

### Changed
- DHT11 驱动改为平台无关协议核心加 STM32F103 端口回调，初始化和释放由页面生命周期显式控制。
- I2C2 总线增加引用计数式 `Acquire/Release` 接口，避免未来多个 I2C 设备之间错误关闭共享总线。
- CS100A 进入页面时检查 TIM2 是否已被占用，退出时恢复进入前的 AFIO JTAG/TIM2 重映射配置。
- `astra_rocket.cpp` 仅保留 Astra 启动和主菜单构建，传感器页面实现迁移至独立页面模块。

### Fixed
- 修复 Distance 页面每次异步采样时在旧距离、`Measuring` 和零值之间切换，导致距离与 Echo 区域按采样频率闪烁的问题。
- CS100A 后台测量期间保留上一笔有效状态和读数，首次进入页面且尚无结果时仍正常显示 `Measuring`。
- BMP280 的 I2C 外设初始化失败统一归类为硬件错误；传感器未响应仍归类为 `No Sensor` 并自动重连。

### Verified
- Keil ARMCC V5.06 全量编译通过，0 Error(s)、0 Warning(s)；资源占用为 Code 41304B、RO-data 4872B、RW-data 240B、ZI-data 17752B。
- 实机顺序初始化、读取和释放三个传感器成功：DHT11 为 26 C/50%，BMP280 为 27.78 C/96471 Pa，CS100A 为 128 mm。
- CS100A 状态诊断确认首次测量完成后，后续 `background_start` 保持 `state=ok raw=ok` 和上一笔有效距离；临时诊断代码已删除。
- STM32CubeProgrammer SWD 下载和校验成功；COM4 硬复位日志确认 `version=v0.4.4`、RAM 17992B/88%、ROM 46416B/71%、Astra 初始化和 FPS 输出正常。

---

## [0.4.3] - 2026-08-21

### Added
- 新增平台无关的 `Hardware/CS100A/cs100a.c/.h`，通过端口回调完成异步测量状态管理、无回波判定和距离换算。
- 新增 `Hardware/CS100A/cs100a_stm32f103.c/.h`，使用 PA15 输出 TRIG、PB3/TIM2_CH2 输入捕获 ECHO，并在退出页面时释放 GPIO、TIM2 和中断资源。
- 新增 `Sensors > Distance` 三级页面，显示实时距离和 ECHO 脉宽，并输出初始化、读数和资源释放串口日志。

### Changed
- 将传感器菜单第三项由 `Light` 改为 `Distance`；CS100A 仅在进入测距页面时初始化，退出页面后停止测量并释放资源。
- 测距过程采用非阻塞 `Start/Poll` 接口，每 500ms 发起一次测量；无有效回波时显示 `No Echo`，端口异常时显示 `Data error`。

### Verified
- Keil ARMCC V5.06 全量编译通过，0 Error(s)、0 Warning(s)；资源占用为 Code 39708B、RO-data 4780B、RW-data 268B、ZI-data 17636B。
- STM32CubeProgrammer SWD 下载和校验成功；COM4 硬复位日志确认 `version=v0.4.3`、看门狗、RAM/ROM、Astra 初始化步骤和 FPS 输出正常。
- 实机测距串口持续输出有效 `pulse_us` 与 `distance_mm`，主循环保持运行且未触发看门狗复位。

---

## [0.4.2] - 2026-08-21

### Added
- 新增独立 `Drivers/I2C/i2c.c/.h`，封装 I2C2 PB10/PB11 初始化、设备探测、寄存器读写和总线释放接口。
- 新增可移植 `Hardware/BMP280/bmp280.c/.h`，通过调用方提供的总线回调完成地址探测、芯片 ID 检查、校准参数读取及温度、气压补偿。
- 新增 `Sensor > Barometer` 三级页面，显示 BMP280 实时温度与气压，并输出地址、芯片 ID、读数和错误状态串口日志。

### Changed
- BMP280 仅在进入 Barometer 页面时初始化，退出页面后进入休眠并释放 I2C2；未进入页面时不访问传感器。
- 温湿度和气压计页面的未连接提示统一为 `No Sensor`，有效通信但数据异常时显示 `Data error`。

### Fixed
- 气压计通信中途断线后释放错误状态下的 I2C2，并每 2 秒重新初始化总线和 BMP280；恢复接线后无需退出页面即可自动恢复数据。

### Verified
- Keil ARMCC V5.06 全量编译通过，0 Error(s)、0 Warning(s)；资源占用为 Code 37696B、RO-data 4748B、RW-data 232B、ZI-data 17616B。
- 实机故障注入验证完成：正常读取后主动关闭 I2C2，串口依次输出 `status=bus_error`、`reconnect status=ok` 和恢复后的有效读数，未触发看门狗复位。
- STM32CubeProgrammer SWD 下载及校验成功，COM4 启动日志、内存日志和 FPS 输出正常。

---

## [0.4.1] - 2026-08-20

### Added
- 新增 `Hardware/Watchdog/watchdog.c/.h`，使用 STM32F103 独立看门狗监测主循环，标称超时时间约 4 秒。
- 启动日志新增看门狗启用状态、标称超时时间和复位原因，能够区分独立看门狗、软件、上电、引脚等复位来源。

### Changed
- 主循环仅在 `astraLoop()` 正常返回后刷新独立看门狗；UI 更新卡死、死循环或 HardFault 持续不返回时自动复位。

### Verified
- Keil ARMCC V5.06 编译通过，0 Error(s)、0 Warning(s)。
- 受控停止刷新看门狗后，设备自动重启且串口打印 `reset_cause=watchdog`；恢复正常刷新后持续运行无误复位。

---

## [0.4.0] - 2026-07-11

### Added
- 新增 `Hardware/DHT11/dht11.c/.h`，封装 PA12 单总线 DHT11 温湿度传感器驱动，包含 GPIO 输入/输出切换、40bit 数据读取、校验和检查和状态码输出。
- 新增主页 `Sensors` 磁贴与传感器二级菜单，当前包含 `Temp/Humi`、`Barometer`、`Light` 三个入口。
- 新增温湿度三级页面，进入后显示 DHT11 温湿度数据；未接入或响应超时时显示 `No sensor`。
- 内容页面新增 enter/exit 生命周期回调，温湿度页面进入时初始化 PA12，退出时释放 PA12，避免未进入页面时持续访问传感器。

### Changed
- 菜单图标从复制到 `std::vector` 改为直接引用 Flash 中的静态图标数据，降低运行期 RAM 占用。
- 主页第二个图标改为传感器图标，第四个标题改为 `Settings` 并保留原始工具图标。
- DHT11 串口日志主状态与屏幕显示统一，例如 `status=No sensor raw=response_high_timeout`，保留 `raw` 字段用于底层问题定位。

### Fixed
- 修复点击温湿度页面后因内容页导航/选择器逻辑不匹配导致的卡死问题。
- 修复传感器未接入、协议错误、校验错误显示不清晰的问题，屏幕与串口主状态保持一致。

---

## [0.3.9] - 2026-07-10

### Changed
- 底部状态栏倒数第二行改为 `RAM:% ROM:%` 百分比显示，最后一行保留左侧运行时间和右侧 FPS。
- 复位启动串口日志新增一次 RAM/ROM 字节数和百分比输出，便于烧录后通过 COM9 验证。

### Fixed
- 修正 ARMCC linker absolute symbol 读取方式，避免 ROM 占用率在屏幕上误显示为 100%。

---

## [0.3.8] - 2026-07-09

### Changed
- **状态栏图标封装** — 将 TF/WiFi/电池图标整理为统一 `StatusIcon` 描述结构，右侧状态栏图标通过统一 helper 从右向左绘制，方便后续替换和复用。
- **状态栏图标重绘** — 重绘 TF 卡、WiFi 和电池区域图标；TF 卡保留斜切缺角与 4px 金属触点槽，WiFi 图标固化为三段同心弧点阵。
- **主页磁贴布局** — 主页 4 个磁贴图标整体下移，减少下方空白；当前选中图标放大显示，光标移走后恢复原尺寸。

### Fixed
- **页面切换闪屏** — 暂停未完成的 `exitAnimation()` 退场遮罩，避免进入/退出二级菜单时偶发改写 canvas 导致闪屏。

---

## [0.3.7] - 2026-07-09

### Changed
- **UI 导航边界** — 关闭菜单循环跳转，光标在首项/末项时不再跨到另一端元素。
- **二级菜单入场** — 每次进入子菜单都重置选中第一项，不再记忆上次退出时的光标位置。

### Fixed
- **入场期间编码器输入** — 先处理输入再渲染，二级菜单元素入场动画未完成时旋转编码器，光标能够立即响应目标项。
- **列表滚动状态残留** — 列表入场时光标绘制位置跟随元素实时位置，但相机滚动判断仍使用最终目标坐标；去掉 `Camera::goToListItemRolling()` 的跨页面 `static direction` 残留，避免下次进入时列表自动滚动。
- **Camera 边界判断** — `Camera::outOfView()` 使用逻辑或 `||` 替代位运算或 `|`，让越界判断更清晰。

---

## [0.3.6] - 2026-07-09

### Changed
- **代码结构整理** — 新增 `app_config.h` 统一维护固件名、版本号、目标芯片、LCD 名称、USART 波特率和当前调试串口配置，避免启动页、状态栏和串口日志版本号分散维护。
- **日志格式统一** — 新增 `app_log.h`，启动日志、Astra 初始化日志、FPS 日志和 HardFault 诊断统一为 `[LEVEL] [module] key=value` 风格，便于串口监听和问题定位。
- **Astra 启动模块解耦** — `astra_rocket.cpp` 拆分开机文字绘制、图标加载、菜单树构建和叶子菜单添加逻辑，并收回内部 `astraLauncher/rootPage/toolPage` 指针，减少外部模块误用。
- **头文件依赖收敛** — `main.h` 移除未直接需要的 USART/LCD 头文件，入口依赖更清晰。
- **脚本注释清理** — `build.bat` / `flash.bat` 注释改为 ASCII，避免 Windows 批处理在非 UTF-8 代码页下把乱码当作命令执行。

### Fixed
- **状态栏版本不一致** — 底部状态栏版本号改为复用 `APP_VERSION`，与开机页和串口启动日志保持一致。
- **过期注释** — `BSP_Init()` 中 KEY 初始化注释更新为当前 KEY1/KEY2 辅助输入说明。
- **文档串口配置** — 当前 USART1 调试串口配置同步为 COM9。

---

## [0.3.5] - 2026-07-08

### Added
- **开机界面** — `astraShowBootScreen()` 居中显示 Dev-Beta / v0.3.5 / STM32F103C8T6 / ST7735S 128x160，持续 2 秒
  - 开机期间通过 `bootScreenActive` 标志跳过顶部/底部状态栏绘制，纯净显示
- `astra_icons.h` — 从 `astra_rocket.cpp` 抽离的 4 个磁贴图标数据 (home/gear/info/tool)

### Changed
- **工程文件解耦** — `astra_rocket.cpp` 不再内联图标数据，改为 include `astra_icons.h`
- `main.cpp` 串口版本号更新为 v0.3.5

### Removed
- **删除废弃代码** `astra_logo.cpp` / `astra_logo.h` (旧 drawLogo 阻塞动画，因 1bpp→RGB565 转换过慢已跳过)

### Fixed
- **LCD 软复位残留** — `LCD_Init()` 在 display on (0x29) 之前增加 `LCD_Fill(0,0,127,159,0x0000)` 清屏，避免复位后短暂闪现上次主页画面

---

## [0.3.4] - 2026-07-08

### Added
- **顶部状态栏右侧图标** — 静态占位图标 (外设未接)
  - 电池图标 16×10 像素 (横向长图标，最右侧)
  - WiFi 图标 8×8 像素
  - SD 卡图标 8×8 像素
- `drawIcon()` 函数 — 支持多字节行宽的位图图标绘制
- **LSE 32.768KHz 晶振配置** — `bsp_init.c` 使能 PC14/OSC32_IN + PC15/OSC32_OUT 外部低速晶振

### Changed
- 顶部状态栏右侧：`T:MMMM` 运行分钟数 → 电池/WiFi/SD卡 静态图标
- 底部状态栏第二行：`STM32F103` + `U:SSSSS` → `HH:MM:SS` 时分秒格式

### Fixed
- **电池图标位置/填充** — 电池移回最右侧，内部白色填充右移 1 像素

---

## [0.3.3] - 2026-07-08

### Added
- **底部常驻状态栏** — 屏幕最下方 32px 区域 (顶部状态栏的 2 倍)，常驻显示不遮挡 UI
  - 第一行：左侧版本号 `v0.3.3`，右侧 `FPS:XX` 帧率
  - 第二行：左侧硬件 `STM32F103`，右侧运行秒数 `U:SSSSS`

### Changed
- `hal.h` `screenHeight` 从 144 改为 **112** (160 - 16 顶部 - 32 底部)
- `hal_port.cpp` 所有绘图原语底部边界从 `ASTRA_SCREEN_H` 改为 `UI_MAX_Y=128`，UI 元素不会侵入底部状态栏
- FPS 帧率计算从 `main.cpp` 移至 `hal_port.cpp` 的 `_canvasUpdate()` 内部
- 串口启动版本号更新为 v0.3.3

### Fixed
- **底部状态栏文字重叠** — 左侧 `Astra v0.3.2` 缩短为 `v0.3.2`，`STM32F103 ST7735S` 缩短为 `STM32F103`，避免与右侧右对齐文字重叠

---

## [0.3.2] - 2026-07-08

### Changed
- **彻底清理 GuiLite 残留** — `font_ascii_8x16.h` 从 `GuiLite/` 迁移到 `Astra/hal/`，精简为仅保留 `font_8x16_data` 数组（移除 GuiLite 专用的 `LATTICE`/`FONT_INFO` 结构和 `font_8x16_init()`）
- `hal_port.cpp` 的 include 从 `"../../GuiLite/font_ascii_8x16.h"` 改为 `"font_ascii_8x16.h"`
- Keil 工程 `IncludePath` 移除 `..\\GuiLite`
- README 版本号同步为 v0.3.2
- 串口启动版本号更新为 v0.3.2

### Removed
- `GuiLite/` 目录彻底删除 — v0.3.0 时仅从 Keil 源文件列表移除未真正删除目录，本次完成清理（`GuiLite.h`/`GuiLite_min.h`/`gfx_types.h`/`GuiLiteAdapter.cpp`/`font_ascii_8x16.h`）

---

## [0.3.1] - 2026-07-08

### Added
- **常驻状态栏** — 顶部 16px 区域显示当前页面标题（左）和运行时间 T:MMMM（右），底部白色分隔线，所有界面常驻不遮挡
- `hal_port.h` 新增 `astraSetStatusBarTitle()` 接口，供 launcher 在页面切换时更新标题
- `main.cpp` LED 心跳（500ms）与 FPS 统计（2000ms）拆分为独立定时器，修复 LED 闪烁变慢问题

### Changed
- `hal.h` `screenHeight` 从 160 改为 144（UI 内容区域），留 16px 给状态栏
- `hal_port.cpp` 所有绘图函数（`_drawPixel`/`_drawHLine`/`_drawVLine`/`_drawBox`）y 坐标加 `UI_OFFSET_Y=16` 偏移，UI 元素自动下移
- `launcher.cpp` init/open/close 后调用 `astraSetStatusBarTitle()` 更新状态栏标题
- 串口启动版本号更新为 v0.3.1

---

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
