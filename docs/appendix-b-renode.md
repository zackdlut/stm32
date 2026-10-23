# 附录 B　Renode 使用说明

本附录说明 `examples/common/` 下的模拟器脚本如何工作、如何为自己的实验编写激励脚本，以及 Renode 的已知限制和常见问题。

## B.1 文件一览

```
examples/common/
├── board.repl     平台描述：在 Renode 自带的 Discovery 板基础上补充 LED、定时器频率、SYSCFG
├── board.resc     启动脚本：创建机器、加载平台、安装时钟钩子、加载 ELF
├── run.sh         make run：无界面运行，收集串口输出
├── sim.sh         make sim：交互式运行，串口转 TCP、开 GDB 服务器
└── lesson.mk      各课共用的 Makefile 规则
```

## B.2 board.repl：平台描述

`.repl` 文件描述“板子上有什么、接在哪里”。我们的文件基于 Renode 自带的 `platforms/boards/stm32f4_discovery-kit.repl`：

```
using "platforms/boards/stm32f4_discovery-kit.repl"

OrangeLED: Miscellaneous.LED @ gpioPortD
RedLED: Miscellaneous.LED @ gpioPortD
BlueLED: Miscellaneous.LED @ gpioPortD

gpioPortD:
    13 -> OrangeLED@0
    14 -> RedLED@0
    15 -> BlueLED@0

timer2:
    frequency: 16000000
...

syscfg: Memory.ArrayMemory @ sysbus 0x40013800
    size: 0x400
```

- 自带描述只连了绿灯（PD12，名为 `UserLED`），这里补上 PD13~15 的另外三个 LED
- `gpioPortD: 13 -> OrangeLED@0` 表示 GPIOD 的第 13 号输出连接到 OrangeLED 的输入 0
- 定时器在构造时设定为 16 MHz（复位后 APB1 定时器时钟），比较通道会继承这个频率
- Renode 没有 SYSCFG 模型（它把每个端口的引脚 N 直接连到 EXTI 线 N），这里用一块普通内存顶替，使 `EXTICR` 的写入能读回

## B.3 board.resc：启动脚本

```
mach create $name
machine LoadPlatformDescription $ORIGIN/board.repl

cpu PerformanceInMips 16
nvic Frequency 16000000

set clock_hook
"""
...Python...
"""
sysbus AddWatchpointHook 0x40023808 DoubleWord Write $clock_hook

logLevel -1 gpioPortD.UserLED
...
macro reset
"""
    sysbus LoadELF $bin
"""
runMacro $reset
```

- `cpu PerformanceInMips 16`：Renode 按“每秒执行多少百万条指令”推进虚拟时间。复位后 CPU 运行在 16 MHz，所以设为 16（相当于每条指令 1 个周期）
- `nvic Frequency`：SysTick 的计数频率
- **时钟钩子**：Renode 的 RCC 模型只报告 PLL 锁定，不会真的改变 CPU 和外设的频率。我们在 `RCC_CFGR`（`0x40023808`）上安装写入监视点，固件每次写这个寄存器时，Python 脚本根据 `RCC_PLLCFGR` 和 `RCC_CFGR` 计算出 HCLK 和 APB1 定时器时钟，然后更新 CPU 速度、SysTick 频率和 TIM2~4 的频率。所以第 6 章切到 168 MHz 后，`delay_ms` 依然准确
- `logLevel -1 gpioPortD.XxxLED`：让 LED 的状态变化以日志形式打印出来
- `macro reset`：Renode 复位机器时会重新执行它，加载 ELF。`sysbus LoadELF` 把各段写入对应地址，并根据向量表设置初始 SP 和 PC

## B.4 run.sh：无界面运行

```bash
"${renode}" --disable-gui --console --plain \
    -e "\$bin=@${elf}" \
    -e "i @${common}/board.resc" \
    -e "sysbus.usart2 CreateFileBackend @${uart_log} true" \
    -e "${run_cmd}" \
    -e "quit" \
    > "${renode_log}" 2>&1 || true
```

1. 设置变量 `$bin` 为 ELF 路径，执行 `board.resc`
2. 把 USART2 的输出写到 `build/uart.log`
3. 如果当前目录有 `stimulus.resc` 就执行它，否则执行 `emulation RunFor "<RUN_TIME>"`（`RUN_TIME` 在各课 Makefile 中设定，默认 1 秒）
4. 退出。完整日志在 `build/renode.log`，屏幕上只显示 LED 变化和错误（`V=1` 时加上警告），然后打印串口输出

## B.5 sim.sh：交互式运行

```bash
exec "${renode}" --disable-gui --console \
    -e "\$bin=@${elf}" \
    -e "i @${common}/board.resc" \
    -e "emulation CreateServerSocketTerminal 3456 \"uart\" false" \
    -e "connector Connect sysbus.usart2 uart" \
    -e "machine StartGdbServer 3333"
```

- USART2 连接到 TCP 端口 3456：`nc localhost 3456`、`telnet localhost 3456` 或 PuTTY（Raw 模式）都可以连
- GDB 服务器监听 3333：`make gdb`
- 机器启动后处于暂停状态，在 Renode 监视器中输入 `start` 开始运行。如果用 GDB 连接，在 GDB 中 `continue` 也会让它运行

## B.6 编写激励脚本

在某一课的目录下创建 `stimulus.resc`，`make run` 就会执行它而不是简单地运行固定时间。脚本里交替使用“运行一段虚拟时间”和“施加输入”：

```
# 运行 100 ms
emulation RunFor "0.1"

# 按下按键（PA0 变高），保持 50 ms，松开
gpioPortA.UserButton Press
emulation RunFor "0.05"
gpioPortA.UserButton Release

# 向串口输入一行（末尾自动加回车）
emulation RunFor "0.1"
sysbus.usart2 WriteLine "led red on"

# 单个字符
sysbus.usart2 WriteChar 0x0D

# 调整日志级别，例如关掉刷屏的 PWM LED
logLevel 3 gpioPortD.UserLED

emulation RunFor "0.5"
```

因为时间是 **虚拟时间**，结果完全可重复：同一个脚本每次运行都得到相同输出，适合用来做自动化测试。第 7 章的抖动模拟就是用 0.5 ms 级别的 `RunFor` 交替按下和松开实现的。

## B.7 已知限制

本教程的例子都已避开或注明下面这些限制：

| 限制 | 影响 | 涉及章节 |
|------|------|---------|
| 没有外设时钟门控 | 忘开 RCC 时钟也能工作，真板上不行 | 4、8 |
| SYSCFG 未模拟 | 用内存顶替；EXTI 固定按引脚号路由 | 7 |
| 不模拟 Flash 等待周期 | 不设 `FLASH->ACR` 也能跑 168 MHz | 6 |
| 时间按指令数估算 | 忙等待延时与真板差别很大 | 4、6 |
| DWT 不模拟 | `CYCCNT` 读出 0 | 11 |
| CPUID 为 `0x410FC240`（r0p0） | 真芯片是 `0x410FC241`（r0p1） | 11 |
| 不模拟波特率 | BRR 写错也能收发 | 8 |
| RAM 上电为 0 | 真芯片是随机值，未初始化变量的 bug 在模拟器里不出现 | 3 |
| OC1PE、部分中断使能位产生警告 | 只是警告，功能正常 | 10 |
| 定时器比较通道频率固定为构造时的值 | 切换时钟后 PWM 时间基准不变 | 10 |
| 未映射地址只警告、返回 0 | 不产生 BusFault | 13 |
| INVSTATE 时栈帧 PC 指向 `blx` 本身 | 真芯片指向跳转目标 | 13 |
| LED 日志时间戳是主机时间 | 不能用来测量虚拟时间间隔；用固件里的 `millis()` | 4 |

## B.8 常见问题

**`make run` 没有任何串口输出**

- 先看 `build/renode.log` 末尾有没有错误（例如 ELF 路径错误、CPU abort）
- 固件是否在打印前就卡住了：用 `make sim` + `make gdb` 看 PC 停在哪里
- 是否配置了 USART2（PA2/PA3，AF7），`CR1` 是否置了 `UE` 和 `TE`

**输出被截断，最后几行没有**

运行时间不够。增大 Makefile 中的 `RUN_TIME`，或在 `stimulus.resc` 末尾增加 `emulation RunFor`。另外 newlib 默认行缓冲，没有 `\n` 的输出要 `fflush(stdout)` 或 `setvbuf(stdout, NULL, _IONBF, 0)`。

**`make sim` 后 Renode 立即退出**

在非交互的环境（例如通过管道或某些 IDE 任务）里运行时，Renode 读到 stdin 的 EOF 就会退出。请在真正的终端中运行 `make sim`。

**提示端口 3333 或 3456 被占用**

上一个 Renode 还在运行。在它的监视器里输入 `quit`，或 `pkill -f Renode` 结束它。

**LED 日志的时间戳间隔和程序里的延时对不上**

日志时间戳是主机的墙上时间，Renode 运行速度可能比真实时间快或慢。比较时间请使用固件打印的 `millis()`，或在监视器中执行 `emulation GetTimeSourceInfo` 查看虚拟时间。

**想看固件对某个外设做了什么**

```
sysbus LogPeripheralAccess sysbus.usart2 true
```

之后每次读写都会记录，例如：

```
usart2: [cpu: 0x80024C6] ReadUInt32 from 0x0 (Status), returned 0xE0
usart2: [cpu: 0x80024CE] ReadUInt32 from 0x4 (Data), returned 0x68
```

再配合 `cpu LogFunctionNames true` 可以看到是哪个函数在访问。

---

[返回目录](README.md)　｜　[附录 A　速查表](appendix-a-cheatsheet.md)
