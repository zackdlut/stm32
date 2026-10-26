# 附录 C　立创·地阔星 STM32F103C8T6 版

正文一直以 STM32F4 Discovery（STM32F407VG）为目标板。如果你手上是 [立创·地阔星 STM32F103C8T6 开发板](https://wiki.lckfb.com/zh-hans/dkx-stm32f103c8t6/)，可以用本附录把第 1~11、13、14、16~19 课跑在这块板子上：每课的 F103 版代码放在该课的 `dkx/` 子目录里，编译时加 `BOARD=dkx` 即可，在 Renode 里也能模拟运行。

本附录只讲 **和正文不同的地方**。原理部分（寄存器是什么、中断怎么工作、I2C 时序……）请照常阅读正文，再回到这里看 F103 上要改什么。

## C.1 板子与 Discovery 的对照

| 项目 | STM32F4 Discovery（正文） | 地阔星（本附录） |
|------|--------------------------|-----------------|
| 芯片 | STM32F407VG | STM32F103C8T6 |
| 内核 | Cortex-M4F（带 FPU） | Cortex-M3（无 FPU，浮点全靠软件） |
| 最高主频 | 168 MHz | 72 MHz |
| Flash / RAM | 1 MB / 192 KB | 64 KB / 20 KB |
| 复位后时钟 | HSI 16 MHz | HSI 8 MHz |
| 外部晶振 | 8 MHz | 8 MHz |
| 用户 LED | PD12~PD15 四个，高电平点亮 | PC13 一个，高电平点亮 |
| 按键 | PA0（B1，按下为高） | 本附录假设 PA0 外接按键到 3.3 V，按下为高 |
| 调试串口 | USART2：PA2 TX、PA3 RX | USART1：PA9 TX、PA10 RX（接 USB-TTL 模块） |
| 下载 / 调试 | 板载 ST-LINK | 4 针 SWD 排针，外接 ST-LINK 或 DAP-Link；也可串口 ISP |
| 参考手册 | RM0090 | **RM0008**（F101/F102/F103/F105/F107） |
| 数据手册 | DS8626 | DS5319（STM32F103x8/xB） |

> LED、按键的极性以你手上板子的原理图为准。若 LED 是低电平点亮，把 `examples/common/dkx/drivers/board.h` 里的 `led_on()` 和 `led_off()` 对调即可；按键接到别的引脚或者按下为低，改 `BUTTON_PORT`、`BUTTON_PIN` 和 `board_init_button()` 里的上下拉方向。

## C.2 怎么用

```bash
cd examples/04-gpio-output
make BOARD=dkx            # 编译 dkx/main.c，输出在 build-dkx/
make BOARD=dkx run        # 在 Renode 里模拟地阔星，打印 USART1 输出
make BOARD=dkx sim        # 交互式：串口走 TCP 3456，GDB 走 3333
make BOARD=dkx sim GUI=1  # 同上，开窗口
make BOARD=dkx clean
```

不加 `BOARD` 时一切和正文一样（`BOARD=disco` 是默认值），F407 的产物仍在 `build/`，两块板子的编译结果互不覆盖。

第 11、16~19 课用到 ST 的 CMSIS 头文件，第一次使用前运行一次下载脚本（会补下 `cmsis-device-f1` 和 `core_cm3.h`，已有的 F4 文件不会重复下载）：

```bash
examples/third_party/fetch.sh
```

第 1 课在模拟器里的输出：

```
$ make BOARD=dkx run
[11:14:13.7200] [NOISY] gpioPortC.UserLED: LED state changed to True
[11:14:13.7486] [NOISY] gpioPortC.UserLED: LED state changed to False
...
----- USART1 output -----
Hello, STM32F103!
blink 1, PC13 LED is on
blink 2, PC13 LED is off
blink 3, PC13 LED is on
blink 4, PC13 LED is off
blink 5, PC13 LED is on
done
```

### 文件布局

```
examples/
├── common/
│   ├── lesson.mk / cmsis.mk     按 BOARD 选择启动文件、链接脚本、编译选项和模拟脚本
│   └── dkx/
│       ├── stm32f103.h          自己写的 F103 寄存器定义（对应正文的 stm32f407.h）
│       ├── startup.c            F103 向量表（16 + 43 项）与 Reset_Handler
│       ├── stm32f103c8.ld       64 KB Flash / 20 KB RAM，段定义复用 stm32f407-sections.ld
│       ├── drivers/             gpio.h、board.h、uart（USART1）、systick
│       ├── cmsis-drivers/       第 16 课起用：基于 stm32f1xx.h 的 board.h、uart、tick
│       ├── board.repl           Renode 平台：LED、按键、RCC/ADC 模型、SHT20、SPI Flash
│       └── board.resc           Renode 启动脚本：时钟钩子、DMA 修正、AFIO 默认映射
└── 04-gpio-output/
    ├── main.c                   F407 版（正文）
    └── dkx/main.c               F103 版（本附录）
```

## C.3 F103 和 F407 最重要的几处不同

把这几条记住，正文里几乎每一课都能自己改到 F103 上。

**1. GPIO 配置寄存器完全不同。** F4 用 `MODER`、`OTYPER`、`OSPEEDR`、`PUPDR`、`AFR` 五组寄存器；F1 每个引脚只有 4 位，放在 `CRL`（引脚 0~7）和 `CRH`（引脚 8~15）里：

每 4 位的高 2 位是 `CNF`、低 2 位是 `MODE`。`MODE = 00` 表示输入，`01/10/11` 表示输出且最高速度为 10/2/50 MHz；`CNF` 在输入时选模拟/浮空/上下拉，在输出时选推挽/开漏、普通/复用。常用组合：

| 4 位的值 | 含义 | 用在 |
|---------|------|------|
| `0x0` | 模拟输入 | ADC |
| `0x4` | 浮空输入（复位状态） | SPI MISO |
| `0x8` | 上拉/下拉输入，方向由 `ODR` 对应位决定：1 上拉，0 下拉 | 按键、USART RX |
| `0x2` | 推挽输出 2 MHz | LED、片选 |
| `0x6` | 开漏输出 2 MHz | |
| `0xB` | 复用推挽 50 MHz | USART TX、SPI SCK/MOSI、PWM |
| `0xF` | 复用开漏 50 MHz | I2C |

- 上下拉没有单独的寄存器：配成 `0x8` 后，写 `ODR` 的对应位选上拉还是下拉。
- 复用功能 **没有 AF 编号**：引脚配成“复用输出”后由它的默认外设驱动（PA9 就是 USART1_TX，PB0 就是 TIM3_CH3）。想换引脚要用 `AFIO_MAPR` 重映射。外设的输入引脚（USART RX、SPI MISO）配成普通输入就行。
- 多了一个 `BRR` 寄存器，写 1 清零对应引脚。

**2. 时钟使能位换了地方。** GPIO 不在 AHB 上，而在 APB2：`RCC->APB2ENR` 的 `IOPAEN`（bit 2）、`IOPBEN`、`IOPCEN`……；USART1、SPI1、ADC1、TIM1、AFIO 也在 APB2 上，USART2、I2C1、TIM2~4 在 APB1 上。

**3. 时钟树更简单。** PLL 没有单独的配置寄存器，来源（`PLLSRC`）、HSE 预分频（`PLLXTPRE`）和倍频系数（`PLLMUL`，2~16）都在 `RCC_CFGR` 里。HSI 进 PLL 前固定除以 2。72 MHz 的标准配置是 HSE 8 MHz × 9，APB1 必须二分频（最高 36 MHz），Flash 要 2 个等待周期。

**4. EXTI 的端口选择在 AFIO 里**（`AFIO->EXTICR[]`，时钟位 `RCC_APB2ENR_AFIOEN`），而不是 SYSCFG。中断号和 F4 一样：EXTI0 = 6，TIM2/3/4 = 28/29/30，USART1/2 = 37/38。

**5. DMA 按“通道”而不是“数据流”组织。** 没有 FIFO、没有 `CHSEL`；每个外设请求固定连到一个通道（RM0008 的 DMA1 request mapping 表），例如 USART1_TX 是 DMA1 通道 4。任意通道置 `MEM2MEM` 位就能做内存到内存拷贝。

**6. ADC 上电后要先校准**，软件启动要把 `EXTSEL` 设成 `111`（SWSTART）并置 `EXTTRIG`；ADC 时钟最高 14 MHz。F103C8 没有 DAC。

**7. 内存小。** 20 KB RAM 里，链接脚本（和 F407 共用的 `stm32f407-sections.ld`）给堆和栈各留了至少 4 KB；第 16 课加上两个 1 KB 的缓冲区，RAM 就用掉了一半多。

## C.4 逐课差异

| 课 | F103 版改了什么 |
|----|----------------|
| 01 | 串口换成 USART1（PA9/PA10），只闪 PC13 一个灯 |
| 02 | GPIOC 的 `ODR` 地址是 `0x4001100C`；PC13 的配置在 `CRH` 的 bit 23:20；演示 `BSRR` 和 `BRR`；`CPUID` 的部件号 `0xC23` 表示 Cortex-M3（模拟器读出 `0x410FC231`，真芯片通常是 r1p1 的 `0x411FC231`） |
| 03 | 地址落在 64 KB Flash、20 KB RAM 内，`_estack = 0x20005000`；向量表 16 + 43 项 |
| 04 | `RCC_APB2ENR`（`0x40021018`）bit 4 开 GPIOC 时钟，`CRH` 写 `0x2` 设为输出 |
| 05 | PA0 配成上下拉输入（`CRL` 写 `0x8`、`ODR0 = 0` 选下拉）；消抖逻辑不变 |
| 06 | 8 MHz HSI → HSE 8 MHz × 9 = 72 MHz：`PLLSRC`、`PLLMUL` 写进 `RCC_CFGR`，APB1 二分频，Flash 2 个等待周期；空循环从 75 ms 降到 8 ms |
| 07 | `AFIO->EXTICR[0]` 选 PA0，其余与正文相同 |
| 08 | 从零写 USART1；它挂在 APB2 上，波特率按 PCLK2 算。8 MHz 下 921600 波特的误差是 −3.5%，72 MHz 下只有 +0.16% |
| 09 | `USART1_IRQHandler`；只有一个灯，命令简化为 `led <on\|off\|toggle>` |
| 10 | PWM 用 **TIM3_CH3 / PB0**（和立创入门手册的呼吸灯一致，外接 LED + 1 kΩ 电阻到 GND），通道 3 的模式在 `CCMR2`；TIM4 每 500 ms 中断翻转 PC13；TIM2 采样 PB0 测占空比。定时器时钟复位后是 8 MHz |
| 11 | `stm32f1xx.h`、`core_cm3.h`、`system_stm32f1xx.c`、`startup_stm32f103xb.s`（C8 和 CB 共用 xb 头文件） |
| 13 | Cortex-M3 的故障模型和 M4 相同（CFSR/HFSR/除零陷阱），五种 `FAULT` 输出一致；`make BOARD=dkx gdb-demo` 使用 `dkx/debug.gdb` |
| 14 | `Pin` 模板在编译期选择 `CRL` 或 `CRH`；用 Morse 码 “SOS” 演示环形缓冲区写满（9 个符号放进 8 格，丢 1 个） |
| 16 | DMA1 通道 1 做内存到内存（`MEM2MEM`），通道 4 做 USART1 发送 |
| 17 | 传感器换成立创手册里的 **SHT20**（地址 0x40，命令和换算公式与 Si7021 相同，结果低 2 位是状态位要清掉）；PB6/PB7 配成复用开漏，F1 引脚在复用模式下没有内部上拉，依赖模块上的上拉电阻 |
| 18 | SPI1：PA5 SCK、PA6 MISO、PA7 MOSI，片选 PA4，外接 W25Q64；SPI 时钟 PCLK2/4 = 2 MHz |
| 19 | ADC 校准、软件触发；外部通道 **IN5 / PA5**（与立创手册的光敏电阻实验一致），内部温度传感器 IN16、内部参考电压 Vrefint IN17；用 Vrefint 反推 VDDA。没有 DAC，也不演示扫描 + DMA |

第 6 课、第 10 课在模拟器里的输出：

```
[HSI 8 MHz] SYSCLK = 8000000 Hz (from RCC registers)
[HSI 8 MHz] busy loop of 100000 iterations took 75 ms
...
[PLL 72 MHz] SYSCLK = 72000000 Hz (from RCC registers)
[PLL 72 MHz] busy loop of 100000 iterations took 8 ms
```

```
TIM3 PWM: PSC=7 ARR=9999 -> 100 Hz
duty set 10%  CCR3= 1000  measured 10%  (t=220 ms)
duty set 25%  CCR3= 2500  measured 25%  (t=440 ms)
duty set 50%  CCR3= 5000  measured 50%  (t=660 ms)
duty set 75%  CCR3= 7500  measured 75%  (t=880 ms)
duty set 90%  CCR3= 9000  measured 90%  (t=1100 ms)
TIM4 interrupts so far: 2 (one per 500 ms)
```

第 19 课（激励脚本依次把 PA5 设为 1241、2482、4095）：

```
lesson 19: ADC1 12-bit, ADCCLK 4000000 Hz, calibrated
Vrefint IN17: raw 1490 -> VDDA = 3298 mV
sensor  IN16: raw 1750 -> 1409 mV -> 29.8 C
single  IN5:  raw 1241 ->  999 mV  (t=2 ms)
single  IN5:  raw 2482 -> 1999 mV  (t=52 ms)
single  IN5:  raw 4095 -> 3298 mV  (t=102 ms)
```

> F1 没有出厂校准的温度系数，温度只能用数据手册的典型值（25 °C 时 1.43 V、4.3 mV/°C）估算，误差可达几度，适合看变化趋势。

### 没有 F103 版的课

| 课 | 原因 / 自己移植的要点 |
|----|---------------------|
| 12 HAL | 需要另外下载 `stm32f1xx-hal-driver`；HAL 的 API 和 F4 几乎一样，主要是 GPIO 初始化结构体的模式名不同 |
| 15 | 本附录 C.6 节就是地阔星版的“上板” |
| 20 RTC 与看门狗 | F1 的 RTC 是一个 32 位秒计数器（`RTC_CNTH/L`），没有 BCD 日历，日期要自己换算；IWDG 用法相同 |
| 21 低功耗 | 模式名相同（Sleep/Stop/Standby），寄存器位在 `PWR_CR`/`SCB->SCR`；唤醒源改用 RTC 闹钟或 EXTI |
| 22 FreeRTOS | 移植层从 `ARM_CM4F` 换成 `ARM_CM3`，`configPRIO_BITS` 仍是 4 |
| 23 Bootloader | F103C8 的 Flash 页只有 1 KB，擦写用 `FLASH_CR` 的 `PER`/`PG`（16 位写入），分区要按 64 KB 重新规划 |
| 24 USB | F103 有 USB 全速设备（PA11/PA12），需要 48 MHz（72 MHz ÷ 1.5）；D+ 的 1.5 kΩ 上拉以原理图为准 |
| 25 测试 | 主机端单元测试与芯片无关，可以直接复用 |

## C.5 与立创入门手册的对照

立创的 [地阔星入门手册](https://wiki.lckfb.com/zh-hans/dkx-stm32f103c8t6/beginner/) 用 Keil + ST 标准外设库（`GPIO_Init()`、`TIM_OC3Init()` 这类函数），本教程用 GCC + 寄存器。两边讲的是同一批外设，可以对照着读：手册告诉你“调哪个库函数”，本教程告诉你“这个函数最终写了哪些寄存器位”。

| 入门手册的内容 | 本教程 |
|---------------|--------|
| 程序下载（DAP-Link、ST-LINK、串口 ISP） | C.6 节 |
| 点灯、GPIO 输出 | 第 4 课 |
| 按键、GPIO 输入 | 第 5 课 |
| 系统时钟、滴答定时器、延时 | 第 6 课 |
| 外部中断 | 第 7 课 |
| 串口收发、printf 重定向 | 第 8、9 课 |
| 定时器中断、“十八、PWM 呼吸灯”（PB0 / TIM3_CH3） | 第 10 课 |
| DMA | 第 16 课 |
| I2C、SHT20 温湿度 | 第 17 课 |
| SPI、W25Q64 Flash | 第 18 课 |
| “二十三、ADC 光敏电阻实验”（PA5 / ADC1_IN5） | 第 19 课 |

手册里的 `RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE)` 就是 `RCC->APB2ENR |= RCC_APB2ENR_IOPBEN`；`GPIO_Mode_AF_PP` + `GPIO_Speed_50MHz` 就是在 `CRL`/`CRH` 里写 `0xB`；`ADC_StartCalibration()` 就是置 `ADC_CR2_CAL` 再等它自己清零。练习：挑手册的一章，在本教程的 F103 版里找到对应的寄存器写法。

## C.6 烧写到真板

接线（SWD 4 针排针）：

```
ST-LINK / DAP-Link         地阔星
   SWDIO  ──────────────  DIO
   SWCLK  ──────────────  CLK
   GND    ──────────────  GND
   3.3V   ──────────────  3V3（板子没有另外供电时）

USB-TTL 模块               地阔星
   TXD    ──────────────►  PA10 (USART1_RX)
   RXD    ◄──────────────  PA9  (USART1_TX)
   GND    ───────────────  GND
```

```bash
cd examples/10-timer-pwm
make BOARD=dkx

# ST-LINK + OpenOCD（DAP-Link 把 interface/stlink.cfg 换成 interface/cmsis-dap.cfg）
openocd -f interface/stlink.cfg -f target/stm32f1x.cfg \
        -c "program build-dkx/firmware.elf verify reset exit"

# 或者 st-flash
st-flash --reset write build-dkx/firmware.bin 0x08000000

# 串口终端
picocom -b 115200 /dev/ttyUSB0
```

**串口 ISP**（不用调试器）：BOOT0 跳线接 1、BOOT1 接 0，按复位进入芯片内置的 bootloader，然后：

```bash
stm32flash -w build-dkx/firmware.bin -v -g 0x08000000 /dev/ttyUSB0
```

烧完把 BOOT0 改回 0 再复位，否则下次上电还会停在 bootloader 里。Windows 上也可以用立创手册介绍的 STM32CubeProgrammer 打开 `build-dkx/firmware.elf`。

在真板上用 GDB 调试（第 13 课）：

```bash
openocd -f interface/stlink.cfg -f target/stm32f1x.cfg      # 终端 1
cd examples/13-debug-fault && make BOARD=dkx FAULT=1        # 终端 2
gdb-multiarch -q build-dkx/fault1/firmware.elf \
    -ex "target extended-remote :3333" -ex "monitor reset halt" -ex "load" -ex "break main" -ex "continue"
```

> 市面上有不少标着 “STM32F103C8T6” 的兼容芯片，OpenOCD 可能报 `UNEXPECTED idcode`。确认接线无误后，可以在 `-f target/stm32f1x.cfg` **之前** 加 `-c "set CPUTAPID 0"` 跳过这项检查。

## C.7 模拟器的局限与板级脚本里的修正

Renode 自带 `platforms/cpus/stm32f103.repl`，但不少外设没有模型或者行为和芯片不一致。`common/dkx/board.repl` 和 `board.resc` 补上了下面这些，读这两个文件时可以对照：

| 方面 | Renode 自带的情况 | 本教程的处理 |
|------|------------------|-------------|
| SVD 文件 | 加载平台时从网上下载，离线会卡住 | 覆盖 `sysbus` 的 `init`，不下载 |
| RCC | 没有模型 | Python 小模型：`...ON` 位自动出现 `...RDY`，`SWS` 跟随 `SW`；写 `RCC_CFGR` 时由钩子重新计算 HCLK 和各定时器时钟（和正文附录 B 的 F4 钩子同理） |
| Flash 接口 | 没有模型 | 一块普通内存，`FLASH_ACR` 能读回 |
| 定时器输出引脚 | 只有固件写了 `AFIO_MAPR` 才把 TIMx 通道接到引脚 | 复位后写一次 `MAPR = 0`，选中芯片的默认映射（TIM3_CH3 → PB0 等） |
| DMA | 把 `CNDTR` 当成字节数；内存到内存、内存到外设的传输在使能瞬间一次完成 | 钩子在使能通道前把 `CNDTR` 换算成字节数，32 位拷贝结果正确；但第 16 课串口 DMA 发送仍是“瞬间完成”，看不到 CPU 并行工作的效果 |
| ADC | 没有模型 | Python 模型：转换立即完成，没有 DMA 请求；各通道的值放在模拟器专用地址 `0x40012400 + 0x100 + 4 × 通道号`，激励脚本用 `sysbus WriteDoubleWord` 修改 |
| SHT20 | 没有模型 | 用 Si7021 模型代替，命令和公式相同；电子 ID 第一字节读出 `0x15` |
| W25Q64 | 没有模型 | 用 Micron MT25Q（8 MB）代替，JEDEC ID 读出 `20 BB 17`，真 W25Q64 是 `EF 40 17`；“不擦除直接改写”在模拟器里会覆盖，真 NOR Flash 只能把 1 改成 0 |
| GPIO 复位值 | `CRL`/`CRH` 读出 0 | 真芯片复位值是 `0x44444444`（全部浮空输入），第 2 课在真板上会看到这个值 |
| 输入引脚写 `ODR` | 忽略并警告 “Trying to set the state of the input pin” | 真芯片上这正是选上下拉的方法；警告可以忽略 |
| DWT 周期计数 | 读出 0 | 真芯片正常（第 11 课） |
| 外设时钟门控、Flash 等待周期、波特率误差 | 都不模拟 | 与正文第 15.5 节相同，上板时逐条检查 |

模拟器里能跑通说明寄存器的 **用法** 是对的；时序、电气特性和上面这些模型不完整的地方，只有真板能验证。

---

[返回目录](README.md)　｜　[附录 B　Renode 使用说明](appendix-b-renode.md)　｜　[第 15 章　迁移到真实硬件](15-real-hardware.md)
