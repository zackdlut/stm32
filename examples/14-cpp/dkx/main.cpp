/*
 * Lesson 14, Dikuoxing STM32F103C8T6 version: modern C++ on a microcontroller.
 *
 * Zero-cost abstractions only: templates, constexpr, RAII and std::array.
 * No heap, no exceptions, no RTTI (see CXXFLAGS in common/lesson.mk).
 * The board has one LED, so the queue holds Morse symbols instead of colours.
 */
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>

#include "drivers/systick.h"
#include "drivers/uart.h"
#include "stm32f103.h"

/* ---- 1. A GPIO pin as a type: port and pin are compile-time constants ---- */
template <std::uintptr_t PortBase, std::uint32_t N>
struct Pin {
    static_assert(N < 16, "a GPIO port has 16 pins");

    static GPIO_TypeDef *port() { return reinterpret_cast<GPIO_TypeDef *>(PortBase); }
    /* Pins 0-7 are configured in CRL, 8-15 in CRH: decided at compile time. */
    static volatile std::uint32_t &config_register() { return N < 8 ? port()->CRL : port()->CRH; }

    static void make_output()
    {
        constexpr std::uint32_t shift = (N & 7u) * 4u;
        config_register() = (config_register() & ~(0xFu << shift)) | (GPIO_CFG_OUT_PP_2MHZ << shift);
    }
    static void set() { port()->BSRR = 1u << N; }
    static void clear() { port()->BRR = 1u << N; }
    static bool is_set() { return (port()->ODR >> N) & 1u; }
    static void toggle() { is_set() ? clear() : set(); }
};

using UserLed = Pin<GPIOC_BASE, 13>;

/* ---- 2. constexpr: computed and checked by the compiler ------------------ */
constexpr std::uint32_t usart_brr(std::uint32_t pclk_hz, std::uint32_t baud)
{
    return (pclk_hz + baud / 2) / baud;
}
static_assert(usart_brr(8'000'000, 115'200) == 0x45, "BRR for 115200 baud at 8 MHz");
static_assert(usart_brr(72'000'000, 115'200) == 625, "BRR for 115200 baud at 72 MHz");

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
enum class Morse : std::uint8_t { Dot, Dash };

static void blink(Morse m)
{
    UserLed::set();
    delay_ms(m == Morse::Dot ? 20 : 60);
    UserLed::clear();
    delay_ms(20);
}

static void tim2_init_10khz()
{
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;
    TIM2->PSC = 0;
    TIM2->ARR = 8'000'000 / 10'000 - 1;
    TIM2->EGR = TIM_EGR_UG;
    TIM2->SR = 0;
    TIM2->DIER = TIM_DIER_UIE;
    nvic_enable_irq(TIM2_IRQn);
    TIM2->CR1 = TIM_CR1_CEN;
}

int main()
{
    uart_init(8'000'000, 115'200);
    systick_init(8'000'000);
    tim2_init_10khz();

    RCC->APB2ENR |= RCC_APB2ENR_IOPCEN;
    UserLed::make_output();

    std::printf("\nC++17 on Cortex-M3\n");
    for (std::size_t i = 0; i < g_boot_count; i++) {
        std::printf("constructor ran before main: %s\n", g_boot_log[i]);
    }
    std::printf("USART1 BRR = 0x%lx (computed at compile time: 0x%lx)\n",
                static_cast<unsigned long>(USART1->BRR), static_cast<unsigned long>(usart_brr(8'000'000, 115'200)));

    /* "SOS" is 9 symbols; the queue holds 8, so the last push fails. */
    RingBuffer<Morse, 8> queue;
    unsigned dropped = 0;
    for (Morse m : {Morse::Dot, Morse::Dot, Morse::Dot, Morse::Dash, Morse::Dash, Morse::Dash,
                    Morse::Dot, Morse::Dot, Morse::Dot}) {
        if (!queue.push(m)) {
            dropped++;
        }
    }
    std::printf("queued %u of %u symbols (%u dropped), sizeof(queue) = %u bytes\n",
                static_cast<unsigned>(queue.size()), static_cast<unsigned>(queue.capacity()), dropped,
                static_cast<unsigned>(sizeof queue));

    static constexpr std::array<const char *, 2> names{"dot", "dash"};
    Morse m;
    while (queue.pop(m)) {
        std::printf("t=%4lu ms  uptime=%7lu us  %s\n", static_cast<unsigned long>(millis()),
                    static_cast<unsigned long>(uptime_us()), names[static_cast<std::size_t>(m)]);
        blink(m);
    }

    std::printf("done\n");
    for (;;) {
        wait_for_interrupt();
    }
}
