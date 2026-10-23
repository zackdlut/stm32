# 附录 A　速查表

本附录汇总 STM32F407 常用的地址、寄存器、公式和命令，方便写代码时查阅。完整内容以 RM0090（参考手册）、DS8626（数据手册）、PM0214（Cortex-M4 编程手册）为准。

## A.1 存储器映射

| 区域 | 起始地址 | 大小 | 说明 |
|------|---------|------|------|
| Flash | `0x0800 0000` | 1 MB | 代码和常量；复位时别名映射到 `0x0000 0000` |
| CCM RAM | `0x1000 0000` | 64 KB | 只有 CPU 能访问（DMA 不行） |
| SRAM1 + SRAM2 | `0x2000 0000` | 112 + 16 KB | 本教程的链接脚本把两者当作 128 KB 连续 RAM |
| APB1 外设 | `0x4000 0000` | | TIM2~7、USART2/3、UART4/5、I2C、SPI2/3…… |
| APB2 外设 | `0x4001 0000` | | TIM1/8/9~11、USART1/6、ADC、SPI1、SYSCFG、EXTI |
| AHB1 外设 | `0x4002 0000` | | GPIOA~I、RCC、Flash 接口、DMA1/2 |
| 内核外设 | `0xE000 E000` | | SysTick、NVIC、SCB（System Control Space） |

## A.2 常用外设基地址

| 外设 | 地址 | 总线 | RCC 使能位 |
|------|------|------|-----------|
| TIM2 / TIM3 / TIM4 / TIM5 | `0x4000 0000` / `0400` / `0800` / `0C00` | APB1 | `APB1ENR` bit 0 / 1 / 2 / 3 |
| TIM6 / TIM7 | `0x4000 1000` / `1400` | APB1 | `APB1ENR` bit 4 / 5 |
| SPI2 | `0x4000 3800` | APB1 | `APB1ENR` bit 14 |
| USART2 / USART3 | `0x4000 4400` / `4800` | APB1 | `APB1ENR` bit 17 / 18 |
| I2C1 | `0x4000 5400` | APB1 | `APB1ENR` bit 21 |
| TIM1 | `0x4001 0000` | APB2 | `APB2ENR` bit 0 |
| USART1 / USART6 | `0x4001 1000` / `1400` | APB2 | `APB2ENR` bit 4 / 5 |
| ADC1 | `0x4001 2000` | APB2 | `APB2ENR` bit 8 |
| SPI1 | `0x4001 3000` | APB2 | `APB2ENR` bit 12 |
| SYSCFG | `0x4001 3800` | APB2 | `APB2ENR` bit 14 |
| EXTI | `0x4001 3C00` | APB2 | 无需使能 |
| GPIOA ~ GPIOE | `0x4002 0000` + 0x400 × n | AHB1 | `AHB1ENR` bit n（A=0、B=1、C=2、D=3、E=4） |
| RCC | `0x4002 3800` | AHB1 | |
| FLASH 接口 | `0x4002 3C00` | AHB1 | |
| DMA1 / DMA2 | `0x4002 6000` / `6400` | AHB1 | `AHB1ENR` bit 21 / 22 |

## A.3 寄存器偏移

**GPIO**

| 偏移 | 寄存器 | 每引脚位数 | 取值 |
|------|--------|-----------|------|
| 0x00 | MODER | 2 | 00 输入、01 输出、10 复用、11 模拟 |
| 0x04 | OTYPER | 1 | 0 推挽、1 开漏 |
| 0x08 | OSPEEDR | 2 | 00 低速 … 11 超高速 |
| 0x0C | PUPDR | 2 | 00 无、01 上拉、10 下拉 |
| 0x10 | IDR | 1 | 输入电平（只读） |
| 0x14 | ODR | 1 | 输出值 |
| 0x18 | BSRR | — | 低 16 位写 1 置位，高 16 位写 1 复位；原子操作 |
| 0x20 / 0x24 | AFR[0] / AFR[1] | 4 | 复用功能号 AF0~AF15，管引脚 0~7 / 8~15 |

**RCC**

| 偏移 | 寄存器 | 用途 |
|------|--------|------|
| 0x00 | CR | HSION/HSIRDY、HSEON/HSERDY、HSEBYP、PLLON/PLLRDY |
| 0x04 | PLLCFGR | PLLM[5:0]、PLLN[14:6]、PLLP[17:16]、PLLSRC[22]、PLLQ[27:24] |
| 0x08 | CFGR | SW[1:0]、SWS[3:2]、HPRE[7:4]、PPRE1[12:10]、PPRE2[15:13] |
| 0x30 | AHB1ENR | GPIO、DMA 时钟 |
| 0x40 | APB1ENR | TIM2~7、USART2/3、I2C、SPI2/3 时钟 |
| 0x44 | APB2ENR | TIM1/8、USART1/6、ADC、SPI1、SYSCFG 时钟 |

**USART**

| 偏移 | 寄存器 | 关键位 |
|------|--------|-------|
| 0x00 | SR | TXE(7)、TC(6)、RXNE(5)、IDLE(4)、ORE(3) |
| 0x04 | DR | 写 = 发送，读 = 接收 |
| 0x08 | BRR | 12.4 定点分频值 |
| 0x0C | CR1 | UE(13)、M(12)、TXEIE(7)、TCIE(6)、RXNEIE(5)、IDLEIE(4)、TE(3)、RE(2) |

**TIM**（通用定时器）

| 偏移 | 寄存器 | 关键位 |
|------|--------|-------|
| 0x00 | CR1 | CEN(0)、ARPE(7) |
| 0x0C | DIER | UIE(0)、CC1IE(1) |
| 0x10 | SR | UIF(0)、CC1IF(1)；**写 0 清零（rc_w0）** |
| 0x14 | EGR | UG(0)：产生更新事件 |
| 0x18 | CCMR1 | OC1M[6:4]（110 = PWM1）、OC1PE(3) |
| 0x20 | CCER | CC1E(0)、CC1P(1) |
| 0x24 / 0x28 / 0x2C | CNT / PSC / ARR | |
| 0x34 ~ 0x40 | CCR1 ~ CCR4 | |

**EXTI / SYSCFG**

| 地址 | 寄存器 | 说明 |
|------|--------|------|
| EXTI + 0x00 | IMR | 中断屏蔽：1 = 允许 |
| EXTI + 0x08 / 0x0C | RTSR / FTSR | 上升沿 / 下降沿触发 |
| EXTI + 0x10 | SWIER | 软件触发 |
| EXTI + 0x14 | PR | 挂起标志：**写 1 清零** |
| SYSCFG + 0x08 ~ 0x14 | EXTICR1 ~ 4 | 每条线 4 位选端口：0 = PA，1 = PB，…… |

**内核外设**

| 地址 | 寄存器 | 说明 |
|------|--------|------|
| `0xE000 E010` | SysTick CTRL | ENABLE(0)、TICKINT(1)、CLKSOURCE(2)、COUNTFLAG(16) |
| `0xE000 E014` / `E018` | SysTick LOAD / VAL | 24 位 |
| `0xE000 E100` | NVIC ISER0~7 | 写 1 使能中断 |
| `0xE000 E180` | NVIC ICER0~7 | 写 1 禁止中断 |
| `0xE000 E200` / `E280` | NVIC ISPR / ICPR | 挂起 / 清除挂起 |
| `0xE000 E400` | NVIC IPR0~59 | 每个中断 1 字节，**高 4 位有效** |
| `0xE000 ED00` | SCB CPUID | |
| `0xE000 ED04` | SCB ICSR | VECTACTIVE[8:0] 当前异常号 |
| `0xE000 ED08` | SCB VTOR | 向量表地址 |
| `0xE000 ED0C` | SCB AIRCR | 写 `0x05FA0004` 软件复位；PRIGROUP[10:8] |
| `0xE000 ED14` | SCB CCR | DIV_0_TRP(4)、UNALIGN_TRP(3) |
| `0xE000 ED18` ~ `ED23` | SCB SHPR1~3 | 系统异常优先级（SysTick 在 `ED23`） |
| `0xE000 ED24` | SCB SHCSR | MEMFAULTENA(16)、BUSFAULTENA(17)、USGFAULTENA(18) |
| `0xE000 ED28` | SCB CFSR | 故障状态（见第 13 章） |
| `0xE000 ED2C` | SCB HFSR | FORCED(30) |
| `0xE000 ED34` / `ED38` | SCB MMFAR / BFAR | 故障地址 |
| `0xE000 ED88` | SCB CPACR | CP10/CP11 = 0b11 打开 FPU |

## A.4 常用中断号

| IRQn | 中断 | IRQn | 中断 |
|------|------|------|------|
| -1 | SysTick（异常 15） | 28 / 29 / 30 | TIM2 / TIM3 / TIM4 |
| 6 ~ 10 | EXTI0 ~ EXTI4 | 35 / 36 | SPI1 / SPI2 |
| 23 | EXTI9_5 | 37 / 38 / 39 | USART1 / USART2 / USART3 |
| 18 | ADC | 40 | EXTI15_10 |
| 31 | I2C1_EV | 50 | TIM5 |

向量表中的位置 = 16 + IRQn，地址偏移 = (16 + IRQn) × 4。

## A.5 位操作

```c
reg |=  (1u << n);                          /* 置位 */
reg &= ~(1u << n);                          /* 清零 */
reg ^=  (1u << n);                          /* 翻转 */
if (reg & (1u << n)) { }                    /* 测试 */

/* 写多位字段：先清后写（读-改-写） */
reg = (reg & ~(MASK << POS)) | ((value & MASK) << POS);

/* GPIO 引脚 n 的 MODER 字段 */
GPIOx->MODER = (GPIOx->MODER & ~(3u << (2 * n))) | (mode << (2 * n));

/* 原子地设置 / 清除引脚 */
GPIOx->BSRR = 1u << n;                      /* 置 1 */
GPIOx->BSRR = 1u << (n + 16);               /* 清 0 */

/* 标志清除的三种方式 */
EXTI->PR = 1u << n;                         /* rc_w1：写 1 清零 */
TIMx->SR = ~TIM_SR_UIF;                     /* rc_w0：写 0 清零 */
(void)USARTx->SR; (void)USARTx->DR;         /* 读序列清零（ORE） */
```

## A.6 公式

| 用途 | 公式 | 例子 |
|------|------|------|
| PLL | SYSCLK = f_src / M × N / P；USB = f_src / M × N / Q | 8 / 8 × 336 / 2 = 168 MHz；/ 7 = 48 MHz |
| VCO 范围 | f_src / M 应为 1~2 MHz；× N 后 100~432 MHz | |
| Flash 等待（3.3 V） | 每 30 MHz 加 1 WS | 168 MHz → 5 WS |
| 定时器时钟 | APB 分频为 1 时 = PCLK，否则 = 2 × PCLK | 168 MHz 配置下 TIM2~7 = 84 MHz |
| 定时器频率 | f = f_TIM / ((PSC + 1) × (ARR + 1)) | 16 MHz / (16000 × 500) = 2 Hz |
| PWM 占空比 | duty = CCR / (ARR + 1) | 5000 / 10000 = 50% |
| SysTick | LOAD = f_HCLK / f_tick − 1 | 16 MHz、1 kHz → 15999 |
| 波特率 | BRR = round(f_PCLK / baud) | 16 MHz、115200 → 139 = `0x8B` |
| 波特率误差 | (f_PCLK / BRR − baud) / baud | −0.08% |

## A.7 本教程的 Make 目标

在任意 `examples/NN-xxx/` 目录下：

| 命令 | 作用 |
|------|------|
| `make` | 编译，生成 `build/firmware.elf`、`.bin`、`.map` |
| `make run` | 在 Renode 中无界面运行，打印 LED 变化和串口输出 |
| `make run V=1` | 同上，并显示 Renode 警告（未模拟的寄存器等） |
| `make sim` | 交互式 Renode：串口在 TCP 3456，GDB 在 3333 |
| `make gdb` | 启动 GDB 并连接 `:3333` |
| `make disasm` | 生成带源码的反汇编 `build/firmware.lst` |
| `make size` | 各段大小 |
| `make clean` | 删除 `build/` |
| `make run FAULT=n` | 第 13 课：选择故障类型 |
| `make gdb-demo` | 第 13 课：自动 GDB 演示 |

## A.8 工具链命令

```bash
arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard ...
arm-none-eabi-objdump -d -S firmware.elf | less        # 反汇编 + 源码
arm-none-eabi-objdump -h firmware.elf                  # 段表
arm-none-eabi-nm -S --size-sort -C firmware.elf        # 符号按大小排序
arm-none-eabi-size firmware.elf                        # text / data / bss
arm-none-eabi-addr2line -f -e firmware.elf 0x08001bc8  # 地址 → 函数和行号
arm-none-eabi-readelf -S firmware.elf                  # ELF 段信息
arm-none-eabi-objcopy -O binary firmware.elf fw.bin    # 生成纯二进制
```

## A.9 GDB

| 命令 | 作用 |
|------|------|
| `target remote :3333` | 连接 GDB 服务器 |
| `monitor reset halt` | （OpenOCD）复位并暂停 |
| `load` | 烧写 ELF |
| `b func` / `b file.c:42` / `tbreak` / `watch var` | 断点 / 临时断点 / 数据断点 |
| `c` / `n` / `s` / `finish` / `si` | 继续 / 单步 / 进入 / 返回 / 单条指令 |
| `bt` / `info args` / `info locals` | 调用栈 / 参数 / 局部变量 |
| `p var` / `p/x var` / `p arr[0]@8` | 打印 |
| `x/8xw 0x40020C00` | 查看内存（8 个 32 位字） |
| `info registers` / `p/x $pc` / `p/x $sp` | CPU 寄存器 |
| `set var x = 5` | 修改变量 |
| `info line *0x08001bc8` | 地址 → 源码行 |
| `disassemble` / `display/i $pc` | 反汇编 |

## A.10 Renode 监视器

| 命令 | 作用 |
|------|------|
| `start` / `pause` / `quit` | 运行 / 暂停 / 退出 |
| `emulation RunFor "0.5"` | 运行 0.5 秒虚拟时间 |
| `peripherals` | 列出所有外设和地址 |
| `sysbus ReadDoubleWord 0x40020C14` | 读 32 位（这里是 GPIOD->ODR） |
| `sysbus WriteDoubleWord 0x40020C18 0x1000` | 写 32 位 |
| `cpu PC` | 当前 PC |
| `gpioPortD.RedLED State` | LED 状态 |
| `gpioPortA.UserButton Press` / `Release` / `PressAndRelease` | 按键 |
| `sysbus.usart2 WriteLine "text"` / `WriteChar 0x41` | 向串口输入 |
| `sysbus LogPeripheralAccess sysbus.usart2 true` | 记录对某外设的每次读写 |
| `cpu LogFunctionNames true` | 记录函数进入（需要 ELF 符号） |
| `logLevel -1 gpioPortD.RedLED` | 调整某外设的日志级别（-1 最详细，3 只显示错误） |

---

[返回目录](README.md)　｜　[附录 B　Renode 使用说明](appendix-b-renode.md)
