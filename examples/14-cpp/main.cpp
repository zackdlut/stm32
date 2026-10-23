/*
 * Lesson 14: modern C++ on a microcontroller.
 *
 * Zero-cost abstractions only: templates, constexpr, RAII and std::array.
 * No heap, no exceptions, no RTTI (see CXXFLAGS in common/lesson.mk).
 */
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>

#include "drivers/systick.h"
#include "drivers/uart.h"
#include "stm32f407.h"

/* ---- 1. A GPIO pin as a type: port and pin are compile-time constants ---- */
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

/* ---- 2. constexpr: computed and checked by the compiler ------------------ */
constexpr std::uint32_t usart_brr(std::uint32_t pclk_hz, std::uint32_t baud)
{
    return (pclk_hz + baud / 2) / baud;
}
static_assert(usart_brr(16'000'000, 115'200) == 0x8B, "BRR for 115200 baud at 16 MHz");
static_assert(usart_brr(42'000'000, 115'200) == 365, "BRR for 115200 baud at 42 MHz");

/* ---- 3. RAII critical section -------------------------------------------- */
class CriticalSection {
public:
    CriticalSection() : primask_(irq_save()) {}
    ~CriticalSection() { irq_restore(primask_); }
    CriticalSection(const CriticalSection &) = delete;
    CriticalSection &operator=(const CriticalSection &) = delete;

private:
    std::uint32_t primask_;
};

/* A 64-bit counter takes two loads on a 32-bit CPU: an interrupt between them
 * would tear the value, hence the critical section in uptime_us(). */
static volatile std::uint64_t g_uptime_us;

extern "C" void TIM2_IRQHandler()
{
    TIM2->SR = ~TIM_SR_UIF;
    g_uptime_us = g_uptime_us + 100;
}

static std::uint64_t uptime_us()
{
    CriticalSection lock;
    return g_uptime_us;
}

/* ---- 4. A ring buffer template with std::array storage ------------------- */
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

/* ---- 5. Global constructors run before main() via __libc_init_array ------ */
static std::array<const char *, 4> g_boot_log{};
static std::size_t g_boot_count = 0;

struct BootStamp {
    explicit BootStamp(const char *name) { g_boot_log[g_boot_count++] = name; }
};

static BootStamp g_first{"first global"};
static BootStamp g_second{"second global"};

/* ---- Application --------------------------------------------------------- */
enum class Color : std::uint8_t { Green, Orange, Red, Blue };

static void show(Color c)
{
    GreenLed::clear();
    OrangeLed::clear();
    RedLed::clear();
    BlueLed::clear();
    switch (c) {
    case Color::Green:  GreenLed::set();  break;
    case Color::Orange: OrangeLed::set(); break;
    case Color::Red:    RedLed::set();    break;
    case Color::Blue:   BlueLed::set();   break;
    }
}

static void tim2_init_10khz()
{
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;
    TIM2->PSC = 0;
    TIM2->ARR = 16'000'000 / 10'000 - 1;
    TIM2->EGR = TIM_EGR_UG;
    TIM2->SR = 0;
    TIM2->DIER = TIM_DIER_UIE;
    nvic_enable_irq(TIM2_IRQn);
    TIM2->CR1 = TIM_CR1_CEN;
}

int main()
{
    uart_init(16'000'000, 115'200);
    systick_init(16'000'000);
    tim2_init_10khz();

    RCC->AHB1ENR |= RCC_AHB1ENR_GPIODEN;
    GreenLed::make_output();
    OrangeLed::make_output();
    RedLed::make_output();
    BlueLed::make_output();

    std::printf("\nC++17 on Cortex-M4\n");
    for (std::size_t i = 0; i < g_boot_count; i++) {
        std::printf("constructor ran before main: %s\n", g_boot_log[i]);
    }
    std::printf("USART2 BRR = 0x%lx (computed at compile time: 0x%lx)\n",
                static_cast<unsigned long>(USART2->BRR), static_cast<unsigned long>(usart_brr(16'000'000, 115'200)));

    RingBuffer<Color, 8> queue;
    for (Color c : {Color::Green, Color::Orange, Color::Red, Color::Blue, Color::Red, Color::Orange}) {
        queue.push(c);
    }
    std::printf("queued %u of %u colors, sizeof(queue) = %u bytes\n", static_cast<unsigned>(queue.size()),
                static_cast<unsigned>(queue.capacity()), static_cast<unsigned>(sizeof queue));

    static constexpr std::array<const char *, 4> names{"green", "orange", "red", "blue"};
    Color c;
    while (queue.pop(c)) {
        show(c);
        std::printf("t=%4lu ms  uptime=%7lu us  LED %s\n", static_cast<unsigned long>(millis()),
                    static_cast<unsigned long>(uptime_us()), names[static_cast<std::size_t>(c)]);
        delay_ms(50);
    }

    show(Color::Green);
    std::printf("done\n");
    for (;;) {
        wait_for_interrupt();
    }
}
