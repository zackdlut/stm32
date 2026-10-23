# 第 14 章　用 C++ 写嵌入式

## 本章目标

- 配置适合单片机的 C++ 编译选项，了解哪些特性要避免
- 用模板、`constexpr`、RAII、`std::array` 写出更安全、但 **零开销** 的驱动代码
- 理解全局构造函数在裸机上是怎么运行的
- 通过反汇编验证抽象确实没有代价

你已经有 C++ 经验，这一章重点是 **嵌入式环境下的取舍**。

## 14.1 编译选项

`common/lesson.mk` 里 C++ 的选项：

```make
CXXFLAGS += $(COMMON_FLAGS) -std=c++17 -fno-exceptions -fno-rtti -fno-threadsafe-statics
LINKER := $(CC)          # 用 gcc 链接，不自动链接 libstdc++
```

| 选项 | 原因 |
|------|------|
| `-fno-exceptions` | 异常需要展开表和运行时支持，增加几十 KB Flash，抛出时间不可预测 |
| `-fno-rtti` | 去掉 `typeid`/`dynamic_cast` 需要的类型信息 |
| `-fno-threadsafe-statics` | 函数内 `static` 对象的初始化默认用 `__cxa_guard_acquire` 加锁；裸机没有线程，不需要 |
| 用 `gcc` 链接 | 不链接 libstdc++，如果不小心用了需要运行时支持的特性（`new`、`std::string`、`iostream`），链接时会报错，而不是悄悄多出 100 KB |

**推荐使用**：类、模板、`constexpr`、`static_assert`、`enum class`、引用、命名空间、lambda、`std::array`、`<type_traits>`、`<cstdint>`、`<algorithm>`（大部分不分配内存）。

**避免使用**：堆分配（`new`、`std::vector`、`std::string`、`std::function`）、异常、`iostream`（单独就有上百 KB）。虚函数可以用，但每次调用多一次间接跳转，且虚表占 Flash。

## 14.2 引脚作为类型

[`examples/14-cpp/main.cpp`](../examples/14-cpp/main.cpp)：

```cpp
template <std::uintptr_t PortBase, std::uint32_t N>
struct Pin {
    static_assert(N < 16, "a GPIO port has 16 pins");

    static GPIO_TypeDef *port() { return reinterpret_cast<GPIO_TypeDef *>(PortBase); }

    static void make_output()
    {
        port()->MODER = (port()->MODER & ~(3u << (N * 2))) | (GPIO_MODE_OUTPUT << (N * 2));
    }
    static void set() { port()->BSRR = 1u << N; }
    static void clear() { port()->BSRR = 1u << (N + 16); }
    static bool is_set() { return (port()->ODR >> N) & 1u; }
    static void toggle() { is_set() ? clear() : set(); }
};

using GreenLed  = Pin<GPIOD_BASE, 12>;
using OrangeLed = Pin<GPIOD_BASE, 13>;
using RedLed    = Pin<GPIOD_BASE, 14>;
using BlueLed   = Pin<GPIOD_BASE, 15>;
```

端口地址和引脚号都是模板参数，也就是编译期常量，所以 `1u << N` 在编译时就算好了。`Pin<GPIOD_BASE, 16>` 会被 `static_assert` 拒绝。

对比 C 版本的 `led_on(uint32_t pin)`：参数在运行时传递，传错端口或引脚号编译器不会发现。

## 14.3 constexpr 与 static_assert

```cpp
constexpr std::uint32_t usart_brr(std::uint32_t pclk_hz, std::uint32_t baud)
{
    return (pclk_hz + baud / 2) / baud;
}
static_assert(usart_brr(16'000'000, 115'200) == 0x8B, "BRR for 115200 baud at 16 MHz");
static_assert(usart_brr(42'000'000, 115'200) == 365, "BRR for 115200 baud at 42 MHz");
```

参数是常量时，函数在 **编译期** 求值，结果直接成为指令里的立即数；`static_assert` 让编译器检查计算结果。还可以检查波特率误差、定时器参数是否超过 16 位等，把一部分运行时 bug 变成编译错误。`16'000'000` 中的单引号是 C++14 的数字分隔符。

## 14.4 RAII 临界区

```cpp
class CriticalSection {
public:
    CriticalSection() : primask_(irq_save()) {}
    ~CriticalSection() { irq_restore(primask_); }
    CriticalSection(const CriticalSection &) = delete;
    CriticalSection &operator=(const CriticalSection &) = delete;

private:
    std::uint32_t primask_;
};

static volatile std::uint64_t g_uptime_us;     /* TIM2 中断每 100 us 加 100 */

static std::uint64_t uptime_us()
{
    CriticalSection lock;
    return g_uptime_us;
}
```

为什么读一个变量也要关中断？32 位 CPU 读 64 位变量需要两次加载。如果在两次加载之间发生中断并修改了变量，就会读到高 32 位是旧值、低 32 位是新值的“撕裂”结果。

RAII 的好处是 **任何返回路径** 都会恢复中断状态，不会因为提前 `return` 忘记开中断。C 版本里每个 `irq_save()` 都要人工配对一个 `irq_restore()`。

中断处理函数要用 `extern "C"`，否则 C++ 的名字修饰（name mangling）会把它变成 `_Z15TIM2_IRQHandlerv`，向量表里的弱符号就不会被覆盖，中断进入默认处理函数：

```cpp
extern "C" void TIM2_IRQHandler()
{
    TIM2->SR = ~TIM_SR_UIF;
    g_uptime_us = g_uptime_us + 100;
}
```

（`g_uptime_us += 100` 在 C++20 中对 volatile 变量已被弃用，所以写成显式的读和写。）

## 14.5 模板环形缓冲区

第 9 章的环形缓冲区改写成模板：

```cpp
template <typename T, std::size_t N>
class RingBuffer {
    static_assert((N & (N - 1)) == 0, "N must be a power of two");

public:
    bool push(const T &value)
    {
        if (size() == N) {
            return false;
        }
        data_[head_++ & (N - 1)] = value;
        return true;
    }
    bool pop(T &out)
    {
        if (size() == 0) {
            return false;
        }
        out = data_[tail_++ & (N - 1)];
        return true;
    }
    std::size_t size() const { return head_ - tail_; }
    static constexpr std::size_t capacity() { return N; }

private:
    std::array<T, N> data_{};
    std::size_t head_ = 0;
    std::size_t tail_ = 0;
};

RingBuffer<Color, 8> queue;      /* sizeof = 8 + 4 + 4 = 16 字节，放在栈上 */
```

`std::array` 是对 C 数组的零开销包装，大小是类型的一部分，没有堆分配。

> 这个版本只在主循环内部使用。如果要在中断和主循环之间共享，需要像第 9 章那样把下标声明为 `volatile`（或 `std::atomic`）并加上编译器屏障。

## 14.6 全局构造函数

```cpp
struct BootStamp {
    explicit BootStamp(const char *name) { g_boot_log[g_boot_count++] = name; }
};

static BootStamp g_first{"first global"};
static BootStamp g_second{"second global"};
```

有非平凡构造函数的全局对象，编译器会生成一个初始化函数，并把它的地址放进 `.init_array` 段。链接脚本收集这个段：

```
.init_array : {
    PROVIDE_HIDDEN(__init_array_start = .);
    KEEP(*(SORT(.init_array.*)))
    KEEP(*(.init_array*))
    PROVIDE_HIDDEN(__init_array_end = .);
} > FLASH
```

`startup.c` 的 `Reset_Handler` 在初始化 `.data`/`.bss` 之后、调用 `main()` 之前调用 newlib 的 `__libc_init_array()`，它依次调用表里的每个函数。这就是第 3 章启动流程里那一步的用途。

注意：**不同源文件之间的全局对象构造顺序是不确定的**（static initialization order fiasco）。构造函数里不要依赖其他文件的全局对象，也不要访问还没开时钟的外设。

## 14.7 运行

```bash
cd examples/14-cpp
make run
```

本机实际输出：

```
C++17 on Cortex-M4
constructor ran before main: first global
constructor ran before main: second global
USART2 BRR = 0x8b (computed at compile time: 0x8b)
queued 6 of 8 colors, sizeof(queue) = 16 bytes
t=   0 ms  uptime=    800 us  LED green
t=  51 ms  uptime=  51000 us  LED orange
t= 101 ms  uptime= 101000 us  LED red
t= 151 ms  uptime= 151000 us  LED blue
t= 201 ms  uptime= 201000 us  LED red
t= 251 ms  uptime= 251100 us  LED orange
done
```

SysTick 计的毫秒和 TIM2 计的微秒互相印证。

## 14.8 验证零开销

```bash
make disasm
```

打开 `build/firmware.lst`。`show()` 和 `uptime_us()` 都被内联进了 `main`：

```
@ show(c)：四个 clear() 各是一条 str，写 BSRR（GPIOD + 24）
 8001ca4:	f04f 5000 	mov.w	r0, #536870912	@ 0x20000000   1 << (13+16)
 8001ca8:	f04f 4180 	mov.w	r1, #1073741824	@ 0x40000000   1 << (14+16)
 8001cac:	f04f 4200 	mov.w	r2, #2147483648	@ 0x80000000   1 << (15+16)
 8001cb0:	61a5      	str	r5, [r4, #24]                    GreenLed::clear()
 8001cb2:	61a0      	str	r0, [r4, #24]                    OrangeLed::clear()
 8001cb4:	61a1      	str	r1, [r4, #24]                    RedLed::clear()
 8001cb6:	61a2      	str	r2, [r4, #24]                    BlueLed::clear()
 8001cb8:	f1b8 0f03 	cmp.w	r8, #3                           switch (c)
 8001cbe:	e8df f008 	tbb	[pc, r8]                         跳转表
 8001cc6:	f44f 4300 	mov.w	r3, #32768	@ 0x8000           BlueLed::set()
 8001cca:	61a3      	str	r3, [r4, #24]

@ uptime_us()：CriticalSection 的构造和析构
 8001cd2:	f3ef 8010 	mrs	r0, PRIMASK                      irq_save()
 8001cd6:	b672      	cpsid	i
 8001cd8:	e9d7 bc00 	ldrd	fp, ip, [r7]                     读 64 位 g_uptime_us
 8001cdc:	f380 8810 	msr	PRIMASK, r0                      irq_restore()
```

模板、内联函数、RAII 对象全部消失，剩下的就是手写寄存器代码会生成的那几条指令。固件大小也印证了这一点：本课 text 段 8.5 KB，和用 C 写的第 11 课（8.4 KB）几乎相同。

## 练习

1. 给 `Pin` 增加 `make_input(Pull pull)`，`Pull` 用 `enum class`。
2. 写一个 `constexpr` 函数计算 BRR 的误差（千分比），用 `static_assert` 保证误差小于 2%，然后试试 921600 波特 @ 16 MHz 会不会编译失败。
3. 把 `RingBuffer` 改成可以在中断和主循环之间安全使用的版本，用于第 9 章的命令行。
4. 写一个函数内 `static` 对象，分别在有和没有 `-fno-threadsafe-statics` 时编译，比较反汇编（看 `__cxa_guard_acquire`）。
5. 用 `std::printf` 之外的方式输出：写一个 `operator<<` 风格的极简串口流类，不依赖 iostream。

## 常见坑

- **中断处理函数忘了 `extern "C"`**：编译链接都没问题，但中断进入 `Default_Handler` 死循环。
- **用了 `new` 或 `std::vector`**：链接报 `undefined reference to operator new`（用 gcc 链接的好处），或者用 g++ 链接时堆从 `_sbrk` 分配，很快耗尽 RAM。
- **全局对象的构造函数访问外设**：此时 `SystemInit` 已运行，但你的时钟和 GPIO 初始化还没运行。
- **`volatile` 与 `+=`**：C++20 起对 volatile 变量的复合赋值被弃用，会有警告。
- **大对象放在栈上**：链接脚本只为栈预留了 4 KB（`_min_stack_size`），`std::array<uint8_t, 4096>` 作为局部变量就会超出，悄悄覆盖堆和全局变量。没有 MMU，不会有任何报错。

---

上一章：[第 13 章　调试与 HardFault](13-debugging.md)　｜　下一章：[第 15 章　迁移到真实硬件](15-real-hardware.md)
