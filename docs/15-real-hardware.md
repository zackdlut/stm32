# 第 15 章　迁移到真实硬件与后续路线

## 本章目标

- 选择一块合适的开发板，了解它和模拟器的对应关系
- 把前面各课的固件烧写到真板上运行和调试
- 了解模拟器和真芯片的差异，知道在真板上要额外注意什么
- 规划后续学习路线

## 15.1 选板

| 开发板 | 芯片 | 与本教程的关系 | 价格（参考） |
|--------|------|---------------|-------------|
| **STM32F4DISCOVERY**（新版型号 STM32F407G-DISC1） | STM32F407VG | **完全一致**：PD12~15 四个 LED、PA0 按键、8 MHz 晶振、板载 ST-LINK | 约 150~200 元 |
| Nucleo-F401RE / F411RE | STM32F401RE / F411RE | 同为 Cortex-M4F，需要改引脚和链接脚本（见 15.5） | 约 100 元 |
| “黑色/蓝色 F407 核心板” + ST-LINK V2 仿真器 | STM32F407VE/ZG | 寄存器相同，LED/按键引脚各家不同，需改 `board.h` | 约 50~100 元 |

首选 Discovery 板：本教程的所有代码不改一行就能运行。

另外准备：

- **USB 转 TTL 串口模块**（CH340、CP2102、FT232 均可，3.3 V 电平）：Discovery 板的 USART2 引出在 PA2/PA3 排针上
- 几根杜邦线

串口接线：

```
USB-TTL 模块          Discovery 板
   TXD   ───────────►  PA3 (USART2_RX)
   RXD   ◄───────────  PA2 (USART2_TX)
   GND   ────────────  GND
（不要接 VCC，板子由 ST-LINK 的 USB 供电）
```

> 新版 STM32F407G-DISC1 的板载 ST-LINK/V2-A 提供虚拟串口，但它是否连到 STM32F407 的某个 USART、需要改哪些锡桥，请以对应版本的 **UM1472** 用户手册为准。外接 USB-TTL 模块最省事。

## 15.2 安装烧写和调试工具

两种常用工具，任选其一：

```bash
# stlink 开源工具：st-flash、st-info
sudo apt install stlink-tools

# OpenOCD：烧写 + GDB 服务器，支持几乎所有调试器
sudo apt install openocd
```

（和第 1 章一样，没有 sudo 时可以用 `apt-get download` + `dpkg-deb -x` 解压到 `~/.local`。）

### WSL2 用户：把 USB 设备接进 WSL

WSL2 默认看不到 USB 设备，需要在 Windows 上安装 [usbipd-win](https://github.com/dorssel/usbipd-win)，然后在 **Windows 管理员 PowerShell** 中：

```powershell
usbipd list                          # 找到 ST-Link 的 BUSID，例如 2-3
usbipd bind --busid 2-3              # 只需执行一次
usbipd attach --wsl --busid 2-3      # 每次插拔后执行
```

回到 WSL 中 `lsusb` 应能看到 `STMicroelectronics ST-LINK/V2`。如果提示权限不足，需要安装 stlink/openocd 自带的 udev 规则（`/etc/udev/rules.d/`），或临时用 sudo 运行。

不想折腾 USB 直通的话，也可以在 Windows 上安装 **STM32CubeProgrammer**，直接打开 WSL 里编译出的 `build/firmware.elf`（路径 `\\wsl$\Ubuntu\home\...`）烧写。

## 15.3 烧写

每一课 `make` 都会生成 `build/firmware.elf`（带调试信息和地址）和 `build/firmware.bin`（纯二进制镜像）。

```bash
cd examples/09-uart-irq-cli
make

# 方法 1：st-flash，bin 文件需要指定地址
st-info --probe                                  # 检测调试器和芯片
st-flash --reset write build/firmware.bin 0x08000000

# 方法 2：OpenOCD，ELF 自带地址
openocd -f interface/stlink.cfg -f target/stm32f4x.cfg \
        -c "program build/firmware.elf verify reset exit"
```

烧写完成后芯片自动复位运行。打开串口终端（波特率 115200，8N1）：

```bash
# Linux / WSL（USB-TTL 模块通常是 /dev/ttyUSB0）
picocom -b 115200 /dev/ttyUSB0          # Ctrl-A Ctrl-X 退出
```

Windows 上用 PuTTY、MobaXterm 或 Tera Term 打开对应的 COM 口即可。在终端里输入 `help`，你会看到和第 9 章 `make sim` 一样的命令行，只是这次点亮的是真的 LED。

## 15.4 在真板上调试

和第 13 章一样，只是 GDB 服务器从 Renode 换成 OpenOCD：

```bash
# 终端 1
openocd -f board/stm32f4discovery.cfg
```

```bash
# 终端 2
cd examples/13-debug-fault
make FAULT=1
gdb-multiarch -q build/fault1/firmware.elf \
    -ex "target extended-remote :3333" \
    -ex "monitor reset halt" \
    -ex "load" \
    -ex "break main" \
    -ex "continue"
```

- `monitor reset halt`：OpenOCD 命令，复位并停住 CPU
- `load`：通过 GDB 把 ELF 烧写到 Flash
- 之后的 `break`、`next`、`print`、`x` 等命令与在 Renode 里完全相同
- 改代码后：`make` 重新编译，在 GDB 里执行 `monitor reset halt`、`load`、`continue`

**VS Code 图形化调试**：安装 Cortex-Debug 扩展，`launch.json` 中设置 `"servertype": "openocd"`、`"executable": "build/firmware.elf"`、`"configFiles": ["board/stm32f4discovery.cfg"]`，即可单步、看变量、看外设寄存器（需要芯片的 SVD 文件）。

## 15.5 模拟器与真芯片的差异

本教程在每章“常见坑”里提到的 Renode 限制汇总如下。代码在模拟器里能跑，不代表在真板上一定能跑，下面这些点要特别检查：

| 方面 | Renode | 真芯片 | 上板时注意 |
|------|--------|--------|-----------|
| 外设时钟门控 | 不模拟，忘开 RCC 时钟也能工作 | 不开时钟，外设寄存器读写无效 | 检查每个外设的 `RCC->xxxENR` |
| Flash 等待周期 | 不模拟 | 168 MHz 下必须设 5 WS，否则取指出错、程序跑飞 | 先设 `FLASH->ACR` 再切时钟（第 6 章） |
| HSE/PLL 起振 | 立即就绪 | 需要几毫秒；晶振坏了会永远等下去 | 等待 RDY 位时加超时 |
| 波特率误差 | 不模拟，BRR 写错也能收发 | 双方误差超过约 2% 就乱码 | 确认 PCLK 和 BRR |
| 按键 | 脚本精确控制 | 有真实抖动，可能持续几毫秒 | 消抖（第 5 章） |
| 执行时间 | 按指令数估算 | 有 Flash 等待、流水线、总线竞争 | 用 DWT 或示波器测量真实时间 |
| DWT 周期计数 | 读出 0 | 正常工作 | 第 11 课可以测到真实周期数 |
| SYSCFG | 未完整模拟 | 正常 | 记得打开 SYSCFG 时钟（第 7 章） |
| 未映射地址 | 警告并返回 0 | BusFault | 第 13 章练习 1 |
| 未初始化 RAM | 通常是 0 | 上电是随机值 | 局部变量一定要初始化 |
| INVSTATE 时的 PC | 指向 `blx` 指令 | 指向跳转目标 | 看 LR 找调用点（第 13 章） |
| GPIO 电气特性 | 理想 | 驱动能力有限、需要上下拉、有上升时间 | 读数据手册的电气参数 |

### 迁移到其他板子

以 Nucleo-F411RE 为例，需要修改：

1. **芯片宏和启动文件**：`-DSTM32F411xE`，使用 `startup_stm32f411xe.s`（中断向量表不同）
2. **链接脚本**：F411RE 是 512 KB Flash、128 KB RAM；F401RE 是 512 KB Flash、96 KB RAM
3. **引脚**：用户 LED LD2 在 **PA5**（高电平点亮），用户按键 B1 在 **PC13**（**低电平有效**，按下为 0，下降沿触发），串口 USART2 PA2/PA3 已连到 ST-LINK 虚拟串口，插上 USB 就有串口
4. **时钟**：Nucleo 板默认没有焊晶振，HSE 来自 ST-LINK 的 8 MHz MCO 输出，需要 **HSE 旁路模式**（`RCC_CR_HSEBYP`）；F411 最高 100 MHz，F401 最高 84 MHz，PLL 参数要重新计算

## 15.6 用 CubeMX 生成工程

实际工作中，初始化代码通常由 **STM32CubeMX**（或集成了它的 **STM32CubeIDE**）生成：

1. 新建工程，选择芯片 STM32F407VGTx（或直接选择 STM32F4DISCOVERY 板）
2. Pinout 页面：点击引脚分配功能，如 PA2 → USART2_TX
3. Clock Configuration 页面：图形化配置时钟树，输入 168 MHz 自动计算 PLL 参数
4. Project Manager：Toolchain 选择 **Makefile** 或 **CMake**，生成代码
5. 生成的工程结构就是第 12 章的样子，在 `/* USER CODE BEGIN */` 和 `/* USER CODE END */` 之间写自己的代码，重新生成时不会被覆盖

学完本教程，你能读懂 CubeMX 生成的每一行代码，以及它背后的寄存器操作。

## 15.7 后续学习路线

按大致顺序：

| 主题 | 要点 | 参考 |
|------|------|------|
| **DMA** | 外设和内存之间自动搬运数据，串口不定长接收（IDLE 中断 + DMA）、ADC 连续采样 | RM0090 “DMA controller” |
| **I2C** | 读写传感器（温湿度、IMU、EEPROM），理解地址、ACK、时钟拉伸 | RM0090 “Inter-integrated circuit (I2C) interface” |
| **SPI** | 驱动显示屏、Flash、SD 卡；Discovery 板上的 LIS3DSH 加速度计就接在 SPI1 上 | RM0090 “Serial peripheral interface (SPI)” |
| **ADC / DAC** | 模拟量采样，配合定时器触发和 DMA | RM0090 “ADC”、“DAC” |
| **RTC、看门狗** | 低功耗计时；IWDG/WWDG 防止程序卡死 | RM0090 “IWDG”、“WWDG”、“RTC” |
| **低功耗** | Sleep / Stop / Standby 模式，唤醒源 | RM0090 “Power controller (PWR)” |
| **FreeRTOS** | 任务、队列、信号量、PendSV/SVC 上下文切换（PSP 栈） | 《Mastering the FreeRTOS Real Time Kernel》 |
| **Bootloader** | 修改 VTOR、跳转到应用、在线升级（IAP） | AN2606、AN3965 |
| **USB** | CDC 虚拟串口、HID | STM32 USB Device Library |
| **单元测试与 CI** | 在 PC 上测试业务逻辑；用 Renode 在 CI 中跑固件测试（Robot Framework） | Renode 文档 |

推荐资料：

- **RM0090** 参考手册：所有外设寄存器的权威说明，本教程的每个寄存器都来自这里
- **DS8626** 数据手册：引脚复用表、电气参数
- **PM0214** 编程手册：Cortex-M4 指令集、NVIC、SCB、故障异常
- **UM1472**：Discovery 板原理图和用户手册
- Joseph Yiu，《The Definitive Guide to ARM Cortex-M3 and Cortex-M4 Processors》：深入理解内核
- Renode 文档：<https://renode.readthedocs.io>

## 练习

1. 把第 4~10 课逐个烧到真板上，记录与模拟器表现不同的地方。
2. 第 4 课的忙等待延时在真板上实际是多长时间？用第 6 课的 SysTick 或示波器测量。
3. 故意删掉第 6 课里设置 `FLASH->ACR` 的那行，在真板上运行会怎样？在 Renode 里呢？
4. 用 CubeMX 生成一个 Discovery 板的 Makefile 工程，让它通过 USART2 打印 `SystemCoreClock`。
5. 读 Discovery 板上 LIS3DSH 加速度计的 WHO_AM_I 寄存器（SPI1：PA5/PA6/PA7，片选 PE3），应读到 `0x3F`（早期 MB997B 版本的板子是 LIS302DL，WHO_AM_I 为 `0x3B`，读之前看清板上的丝印）。

---

上一章：[第 14 章　用 C++ 写嵌入式](14-cpp.md)　｜　[返回目录](README.md)　｜　附录：[A 速查表](appendix-a-cheatsheet.md)、[B Renode 使用说明](appendix-b-renode.md)
