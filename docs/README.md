# STM32 从零到上手：基于 Renode 模拟器的实战教程

这是一套面向 **有 C/C++ 经验、但没接触过单片机** 的开发者的 STM32 入门教程。全部例子都能在本机的 Renode 模拟器里运行，不需要买开发板；学完后换到真实硬件只需改烧录方式（第 15 章）。

- 目标芯片：**STM32F407VG**（Cortex-M4F，168 MHz，1 MB Flash，192 KB RAM）
- 目标板：**STM32F4 Discovery**（4 个 LED 接 PD12~PD15，用户按键接 PA0）
- 工具：`arm-none-eabi-gcc`、`make`、Renode、`gdb-multiarch`
- 另一块板子：第 1~11、13、14、16~19 课还有 **立创·地阔星 STM32F103C8T6** 版本，`make BOARD=dkx run` 即可，见 [附录 C](appendix-c-dkx-f103.md)
- 每一课都有可运行代码（`examples/`）、原理图（`docs/images/`）和实际运行输出

![从源代码到芯片](images/01-toolchain-flow.svg)

## 学习路线

```
 基础篇（裸寄存器）                          进阶篇（中断与外设）                    工程篇
┌─────────────────────────┐   ┌──────────────────────────────┐   ┌──────────────────────────┐
│ 01 环境搭建              │   │ 06 时钟树与 SysTick           │   │ 11 CMSIS 官方头文件       │
│ 02 MCU 与寄存器          │──▶│ 07 中断、NVIC 与 EXTI         │──▶│ 12 HAL 库与 CubeMX 结构   │
│ 03 启动过程与链接脚本    │   │ 08 UART 串口与 printf         │   │ 13 GDB 调试与故障分析     │
│ 04 GPIO 输出：点灯       │   │ 09 串口中断 + 命令行          │   │ 14 嵌入式 C++             │
│ 05 GPIO 输入：按键消抖   │   │ 10 定时器与 PWM               │   │ 15 走向真实硬件           │
└─────────────────────────┘   └──────────────────────────────┘   └────────────┬─────────────┘
            ┌─────────────────────────────────────────────────────────────────────┘
            ▼
 外设篇（CMSIS 头文件）          系统篇
┌─────────────────────────┐   ┌──────────────────────────────┐
│ 16 DMA                  │   │ 21 低功耗                     │
│ 17 I2C 总线             │──▶│ 22 FreeRTOS                   │
│ 18 SPI 总线             │   │ 23 Bootloader 与固件升级       │
│ 19 ADC 与 DAC           │   │ 24 USB 虚拟串口                │
│ 20 RTC 与看门狗         │   │ 25 单元测试与 CI               │
└─────────────────────────┘   └──────────────────────────────┘
```

前 10 章刻意 **不用任何厂商库**，只用自己写的寄存器头文件，让你看清每一个比特；第 11、12 章再引入 ST 官方的 CMSIS 和 HAL，你会发现它们只是对同一批寄存器的封装。第 16 章起直接用 CMSIS 头文件（`stm32f4xx.h`）操作寄存器，这也是实际项目里最常见的写法。

外设篇和系统篇依赖第 1~11 章的知识，各章之间基本独立，可以按需挑着读；第 22 章 FreeRTOS 建议在第 21 章之后读，第 25 章用到第 9 章的命令行思路。第 24 章 USB 需要真实开发板，其余各章都能在模拟器里完整运行。

## 章节目录

| 章 | 文档 | 例子 | 你会学到 |
|----|------|------|---------|
| 1 | [环境搭建与第一个程序](01-setup.md) | `01-hello` | 工具链、Renode、`make run` |
| 2 | [MCU 基础：存储器映射与寄存器](02-mcu-basics.md) | `02-registers` | 地址即寄存器、位操作、`volatile` |
| 3 | [启动过程与链接脚本](03-startup.md) | `03-startup` | 向量表、`.data/.bss`、`main` 之前 |
| 4 | [GPIO 输出：点亮 LED](04-gpio-output.md) | `04-gpio-output` | RCC 时钟、MODER、ODR、BSRR |
| 5 | [GPIO 输入：按键与消抖](05-gpio-input.md) | `05-gpio-input` | IDR、上下拉、抖动、轮询 |
| 6 | [时钟树与 SysTick](06-clock-systick.md) | `06-clock-systick` | HSI/HSE/PLL、168 MHz、1 ms 节拍 |
| 7 | [中断：NVIC 与 EXTI](07-interrupts-exti.md) | `07-exti-interrupt` | 向量表、优先级、临界区、WFI |
| 8 | [UART 串口与 printf](08-uart.md) | `08-uart-printf` | 帧格式、波特率、newlib 重定向 |
| 9 | [串口中断与命令行](09-uart-interrupt-cli.md) | `09-uart-irq-cli` | RXNE 中断、环形缓冲区、CLI |
| 10 | [定时器与 PWM](10-timers-pwm.md) | `10-timer-pwm` | PSC/ARR/CCR、周期中断、调光 |
| 11 | [CMSIS 官方头文件](11-cmsis.md) | `11-cmsis` | `stm32f4xx.h`、`core_cm4.h`、`SystemInit` |
| 12 | [HAL 库](12-hal.md) | `12-hal` | 句柄、MSP、回调、CubeMX 工程结构 |
| 13 | [调试与故障分析](13-debugging.md) | `13-debug-fault` | GDB、HardFault、CFSR、addr2line |
| 14 | [嵌入式 C++](14-cpp.md) | `14-cpp` | 模板、`constexpr`、RAII、零开销 |
| 15 | [走向真实硬件](15-real-hardware.md) | — | 烧录、调试探头、仿真与实物差异、进阶路线 |
| 16 | [DMA](16-dma.md) | `16-dma` | 数据流/通道、内存到内存、串口 DMA 发送、中断标志 |
| 17 | [I2C 总线](17-i2c.md) | `17-i2c` | 起始/停止、地址与 ACK、读 Si7021 温湿度 |
| 18 | [SPI 总线](18-spi.md) | `18-spi` | CPOL/CPHA、片选、读写 SPI NOR Flash |
| 19 | [ADC 与 DAC](19-adc-dac.md) | `19-adc-dac` | 逐次逼近、单次/扫描 + DMA、DAC 输出、自定义外设模型 |
| 20 | [RTC 与看门狗](20-rtc-watchdog.md) | `20-rtc-watchdog` | LSE/备份域、BCD 日历、闹钟、IWDG、复位原因 |
| 21 | [低功耗](21-low-power.md) | `21-low-power` | Sleep/Stop/Standby、RTC 唤醒、用指令数比较功耗 |
| 22 | [FreeRTOS](22-freertos.md) | `22-freertos` | 任务、队列、信号量、互斥锁、PendSV、中断优先级 |
| 23 | [Bootloader 与固件升级](23-bootloader.md) | `23-bootloader` | 分区、镜像头 + CRC、擦写 Flash、VTOR 跳转、断电安全升级 |
| 24 | [USB 虚拟串口](24-usb.md) | `24-usb-cdc` | 端点、描述符、枚举、48 MHz、TinyUSB CDC（需真板） |
| 25 | [单元测试与 CI](25-testing-ci.md) | `25-testing` | Unity、fake、Sanitizer、覆盖率、Robot 系统测试、GitHub Actions |
| 附录 A | [速查表](appendix-a-cheatsheet.md) | — | 地址、寄存器、公式、命令 |
| 附录 B | [Renode 使用说明、局限与 FAQ](appendix-b-renode.md) | — | 脚本、模拟按键/串口、常见问题 |
| 附录 C | [立创·地阔星 STM32F103C8T6 版](appendix-c-dkx-f103.md) | 各课 `dkx/` | `BOARD=dkx`、F103 与 F407 的差异、逐课改动、上板烧写 |

## 怎么使用这套教程

1. **按顺序读**。每章开头有“本章目标”，结尾有“练习”和“常见坑”。
2. **边读边跑**。每章都给出命令和 **真实的运行输出**（在本机 Renode 1.17.0 上跑出来的），你的输出应当一致。
3. **改代码再跑**。练习题都可以直接在例子目录里改，`make run` 立即看到结果。
4. **遇到不懂的寄存器**，查附录 A 或 ST 参考手册 RM0090 对应章节。

每一课的标准操作：

```bash
cd examples/04-gpio-output   # 进入某一课
make run                     # 编译 + 在 Renode 里运行 + 打印串口输出
make disasm                  # 生成反汇编 build/firmware.lst
make sim                     # 交互式运行（串口走 TCP 3456，GDB 走 3333）
make sim GUI=1               # 同上，监视器和串口终端开成窗口
make clean                   # 清理
```

## 前置知识

- 必须：C 语言（指针、位运算、结构体、`static`/`extern`）、会用 Linux 命令行和 `make`
- 有帮助：C++（第 14 章）、一点数字电路常识（高/低电平、上拉电阻）
- 不需要：汇编、电子电路设计、任何 STM32 经验

## 目录结构

```
stm32/
├── docs/                      本教程
│   ├── README.md
│   ├── 01-setup.md … 25-testing-ci.md
│   ├── appendix-*.md
│   └── images/*.svg           原理图
└── examples/
    ├── common/                所有课共用
    │   ├── stm32f407.h        自己写的寄存器定义（第 2 章讲）
    │   ├── startup.c          启动代码与向量表（第 3 章讲）
    │   ├── stm32f407.ld       链接脚本：存储器布局（第 3 章讲）
    │   ├── stm32f407-sections.ld  链接脚本：段定义，第 23 章的 bootloader/app 也复用
    │   ├── syscalls.c         newlib 系统调用（第 8 章讲）
    │   ├── drivers/           从各课提炼出的小驱动：gpio/uart/systick/board
    │   ├── cmsis-drivers/     第 16 章起用的 CMSIS 版小驱动：board.h、uart、tick
    │   ├── lesson.mk          公共 Makefile 规则
    │   ├── cmsis.mk           第 16 章起的 Makefile 规则（ST 启动文件 + CMSIS）
    │   ├── board.repl/.resc   Renode 板级描述与启动脚本（含 I2C 传感器、SPI Flash、DAC 模型）
    │   ├── run.sh             无界面运行并收集串口输出（LOG_FILTER、STIMULUS 可调）
    │   ├── sim.sh             交互式运行
    │   └── dkx/               地阔星 STM32F103 的头文件、启动文件、驱动和 Renode 描述（附录 C）
    ├── 01-hello … 25-testing/ 每课一个目录：main.c + Makefile (+ stimulus.resc)，部分课有 dkx/ 子目录
    ├── third_party/fetch.sh   下载 CMSIS（F4、F1）、HAL、FreeRTOS、TinyUSB、Unity
    └── stm32f407-renode/      最早的冒烟测试工程
```

## 参考资料（建议下载备查）

| 文档 | 内容 |
|------|------|
| RM0090 Reference Manual | STM32F405/407 全部外设寄存器，**最常翻** |
| DS8626 Datasheet (STM32F407xx) | 引脚复用表（AF 表）、电气参数 |
| PM0214 Programming Manual | Cortex-M4 内核：指令、NVIC、SysTick、故障寄存器 |
| UM1472 User Manual | STM32F4 Discovery 板原理图与接线 |
| Joseph Yiu《The Definitive Guide to ARM Cortex-M3 and Cortex-M4 Processors》 | 内核原理最好的书 |

准备好了就从 [第 1 章：环境搭建](01-setup.md) 开始。
