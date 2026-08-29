/**
 * @file uart0.cpp
 *
 */
/* Copyright (C) 2023-2026 by Arjan van Vught mailto:info@gd32-dmx.org
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:

 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <cstdint>

#include "gd32.h"

#if defined(GD32H7XX) // GD32H7XX
#define TIMERx TIMER15
#define RCU_TIMERx RCU_TIMER15
#define TIMERx_IRQHandler TIMER15_IRQHandler
#define TIMERx_IRQn TIMER15_IRQn
#elif defined(GD32F30X) // GD32F30X
#define TIMERx TIMER7
#define RCU_TIMERx RCU_TIMER7
#if defined(GD32F30X_XD) // GD32F30X_XD
#define TIMERx_IRQHandler TIMER7_UP_TIMER12_IRQHandler
#define TIMERx_IRQn TIMER7_UP_TIMER12_IRQn
#else
#define TIMERx_IRQHandler TIMER7_UP_IRQHandler
#define TIMERx_IRQn TIMER7_UP_IRQn
#endif // GD32F30X_XD
#else
#define TIMERx TIMER9
#define RCU_TIMERx RCU_TIMER9
#define TIMERx_IRQHandler TIMER0_UP_TIMER9_IRQHandler
#define TIMERx_IRQn TIMER0_UP_TIMER9_IRQn
#endif // GD32H7XX

#if defined(GD32H7XX)
#define TIMER_CLOCK_FREQ (AHB_CLOCK_FREQ)
#elif defined(GD32F4XX)
#define TIMER_CLOCK_FREQ (APB2_CLOCK_FREQ * 2)
#else
#define TIMER_CLOCK_FREQ (APB2_CLOCK_FREQ)
#endif // GD32H7XX

#if !defined(SOFTUART_TX_PINx)
#define SOFTUART_TX_PINx GPIO_PIN_9
#define SOFTUART_TX_GPIOx GPIOA
#define SOFTUART_TX_RCU_GPIOx RCU_GPIOA
#endif // SOFTUART_TX_PINx

#if defined(SOFTUART0_ENABLE_RX)
#if !defined(SOFTUART_RX_PINx)
#define SOFTUART_RX_PINx GPIO_PIN_10
#define SOFTUART_RX_GPIOx GPIOA
#define SOFTUART_RX_RCU_GPIOx RCU_GPIOA
#if defined(GD32H7XX)
#error
#else
#define SOFTUART_RX_TIMERx TIMER11
#define SOFTUART_RX_RCU_TIMERx RCU_TIMER11
#define SOFTUART_RX_TIMERx_IRQHandler TIMER7_BRK_TIMER11_IRQHandler
#define SOFTUART_RX_TIMERx_IRQn TIMER7_BRK_TIMER11_IRQn
#define SOFTUART_RX_EXTIx EXTI_10
#define SOFTUART_RX_EXTIx_IRQHandler EXTI10_15_IRQHandler
#define SOFTUART_RX_EXTIx_IRQn EXTI10_15_IRQn
#define SOFTUART_RX_GPIO_PORT_SOURCE_GPIOx GPIO_PORT_SOURCE_GPIOA
#define SOFTUART_RX_GPIO_PIN_SOURCE_x GPIO_PIN_SOURCE_14
#endif // GD32H7XX
#endif // SOFTUART_RX_PINx
static_assert(TIMERx != SOFTUART_RX_TIMERx);
#endif // SOFTUART0_ENABLE_RX

namespace {
constexpr uint32_t kBaudRate = 115200;
constexpr uint8_t kBitsPerFrame = 8;
constexpr uint32_t kTimerPeriod = ((TIMER_CLOCK_FREQ / kBaudRate) - 1U);
constexpr uint32_t kBufferSize = 128;

enum class TxState { kIdle, kStartBit, kData, kStopBit };
enum class RxState { kIdle, kStart, kData, kStop };

struct CircularBuffer {
    uint8_t buffer[kBufferSize];
    uint32_t head;
    uint32_t tail;
};

volatile CircularBuffer s_tx_buffer __attribute__((aligned(4)));
volatile TxState s_tx_state;
volatile uint8_t s_tx_data;
volatile uint8_t s_tx_shift;

#if defined(SOFTUART0_ENABLE_RX)
volatile CircularBuffer s_rx_buffer __attribute__((aligned(4)));
volatile RxState s_rx_state;
volatile uint8_t s_rx_data;
volatile uint8_t s_rx_shift;
#endif // SOFTUART0_ENABLE_RX
} // namespace

extern "C" {
// TX
void TIMERx_IRQHandler() {
    const auto kIntFlag = TIMER_INTF(TIMERx);

    if ((kIntFlag & TIMER_INT_FLAG_UP) == TIMER_INT_FLAG_UP) [[likely]] {
        switch (s_tx_state) {
            case TxState::kStartBit:
                GPIO_BC(SOFTUART_TX_GPIOx) = SOFTUART_TX_PINx;

                s_tx_state = TxState::kData;
                s_tx_data = s_tx_buffer.buffer[s_tx_buffer.tail];
                s_tx_buffer.tail = (s_tx_buffer.tail + 1) & (kBufferSize - 1);
                s_tx_shift = 0;
                break;
            case TxState::kData:
                if ((s_tx_data & (1U << s_tx_shift)) != 0) {
                    GPIO_BOP(SOFTUART_TX_GPIOx) = SOFTUART_TX_PINx;
                } else {
                    GPIO_BC(SOFTUART_TX_GPIOx) = SOFTUART_TX_PINx;
                }

                s_tx_shift = s_tx_shift + 1;

                if (s_tx_shift == kBitsPerFrame) {
                    s_tx_state = TxState::kStopBit;
                }
                break;
            case TxState::kStopBit:
                GPIO_BOP(SOFTUART_TX_GPIOx) = SOFTUART_TX_PINx;

                if (s_tx_buffer.head == s_tx_buffer.tail) {
                    s_tx_state = TxState::kIdle;
                    timer_disable(TIMERx);
                } else {
                    s_tx_state = TxState::kStartBit;
                }
                break;
            default:
                break;
        }
    }

    TIMER_INTF(TIMERx) = ~kIntFlag;
}

#if defined(SOFTUART0_ENABLE_RX)
void SOFTUART_RX_EXTIx_IRQHandler() {
    if (RESET == exti_interrupt_flag_get(SOFTUART_RX_EXTIx)) [[unlikely]] {
        return;
    }

    EXTI_PD = SOFTUART_RX_EXTIx; // Interrupt flag clear

    if (s_rx_state != RxState::kIdle) [[unlikely]] {
        return;
    }

    if ((GPIO_ISTAT(SOFTUART_RX_GPIOx) & SOFTUART_RX_PINx) != 0U) [[unlikely]] { // RX pin must be low
        return;
    }

    s_rx_state = RxState::kStart;
    s_rx_data = 0;
    s_rx_shift = 0;

    auto exti_int = EXTI_INTEN;
    exti_int &= static_cast<uint32_t>(~SOFTUART_RX_EXTIx);
    EXTI_INTEN = exti_int;

    auto ctl0 = TIMER_CTL0(SOFTUART_RX_TIMERx);
    ctl0 &= ~TIMER_CTL0_CEN;
    TIMER_CTL0(SOFTUART_RX_TIMERx) = ctl0;
    TIMER_CNT(SOFTUART_RX_TIMERx) = 0;
    TIMER_INTF(SOFTUART_RX_TIMERx) = UINT32_MAX;
    TIMER_CAR(SOFTUART_RX_TIMERx) = kTimerPeriod / 2;
    ctl0 |= TIMER_CTL0_CEN;
    TIMER_CTL0(SOFTUART_RX_TIMERx) = ctl0;
}

void SOFTUART_RX_TIMERx_IRQHandler() {
    const auto kIntFlag = TIMER_INTF(SOFTUART_RX_TIMERx);

    if ((kIntFlag & TIMER_INT_FLAG_CH0) == TIMER_INT_FLAG_CH0) [[likely]] {
        switch (s_rx_state) {
            case RxState::kIdle:
                if ((GPIO_ISTAT(SOFTUART_RX_GPIOx) & SOFTUART_RX_PINx) != 0U) {
                    auto ctl0 = TIMER_CTL0(SOFTUART_RX_TIMERx);
                    ctl0 &= ~TIMER_CTL0_CEN;
                    TIMER_CTL0(SOFTUART_RX_TIMERx) = ctl0;
                    auto int_enable = EXTI_INTEN;
                    int_enable |= SOFTUART_RX_EXTIx;
                    EXTI_INTEN = int_enable;
                    break;
                }

                s_rx_state = RxState::kData;
                break;
            case RxState::kStart: {
                auto ctl0 = TIMER_CTL0(SOFTUART_RX_TIMERx);
                ctl0 &= ~TIMER_CTL0_CEN;
                TIMER_CTL0(SOFTUART_RX_TIMERx) = ctl0;
                TIMER_CNT(SOFTUART_RX_TIMERx) = 0;
                TIMER_CAR(SOFTUART_RX_TIMERx) = kTimerPeriod;
                ctl0 |= TIMER_CTL0_CEN;
                TIMER_CTL0(SOFTUART_RX_TIMERx) = ctl0;
            }
                s_rx_state = RxState::kData;
                break;
            case RxState::kData:
                if ((GPIO_ISTAT(SOFTUART_RX_GPIOx) & SOFTUART_RX_PINx) != 0U) {
                    s_rx_data = s_rx_data | (1U << s_rx_shift);
                }

                s_rx_shift = s_rx_shift + 1;

                if (s_rx_shift == kBitsPerFrame) {
                    s_rx_state = RxState::kStop;
                }
                break;
            case RxState::kStop:
                if ((GPIO_ISTAT(SOFTUART_RX_GPIOx) & SOFTUART_RX_PINx) != 0U) {
                    s_rx_buffer.buffer[s_rx_buffer.head] = s_rx_data;
                    s_rx_buffer.head = s_rx_buffer.head + 1;
                }

                s_rx_data = 0;
                s_rx_shift = 0;
                s_rx_state = RxState::kIdle;
                break;
        }
    }

    TIMER_INTF(SOFTUART_RX_TIMERx) = ~kIntFlag;
}
#endif // SOFTUART0_ENABLE_RX
}

namespace uart0 {
namespace {
auto is_init = false;

void GpioConfig() {
    rcu_periph_clock_enable(SOFTUART_TX_RCU_GPIOx);

#if defined(GPIO_INIT)
    gpio_init(SOFTUART_TX_GPIOx, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, SOFTUART_TX_PINx);
#else
    gpio_mode_set(SOFTUART_TX_GPIOx, GPIO_MODE_OUTPUT, GPIO_PUPD_PULLDOWN, SOFTUART_TX_PINx);
    gpio_output_options_set(SOFTUART_TX_GPIOx, GPIO_OTYPE_PP, GPIO_OSPEED, SOFTUART_TX_PINx);
#endif // GPIO_INIT

#if defined(SOFTUART0_ENABLE_RX)
    rcu_periph_clock_enable(SOFTUART_RX_RCU_GPIOx);
#if defined(GPIO_INIT)
    gpio_init(SOFTUART_RX_GPIOx, GPIO_MODE_IPU, GPIO_OSPEED_50MHZ, SOFTUART_RX_PINx);
    rcu_periph_clock_enable(RCU_AF);
#else
    rcu_periph_clock_enable(RCU_SYSCFG);
    gpio_mode_set(SOFTUART_RX_GPIOx, GPIO_MODE_INPUT, GPIO_PUPD_PULLUP, SOFTUART_RX_PINx);
#endif // GPIO_INIT
    exti_init(SOFTUART_RX_EXTIx, EXTI_INTERRUPT, EXTI_TRIG_FALLING);
    exti_interrupt_flag_clear(SOFTUART_RX_EXTIx);

    NVIC_SetPriority(SOFTUART_RX_EXTIx_IRQn, (1UL << __NVIC_PRIO_BITS) - 1UL); // Lowest priority
    NVIC_EnableIRQ(SOFTUART_RX_EXTIx_IRQn);
#endif // SOFTUART0_ENABLE_RX

    GPIO_BOP(SOFTUART_TX_GPIOx) = SOFTUART_TX_PINx;
}

void TimersConfig() {
    rcu_periph_clock_enable(RCU_TIMERx);
    timer_deinit(TIMERx);
    timer_parameter_struct timer_initpara;
    timer_struct_para_init(&timer_initpara);

    timer_initpara.prescaler = 0;
    timer_initpara.alignedmode = TIMER_COUNTER_EDGE;
    timer_initpara.counterdirection = TIMER_COUNTER_UP;
    timer_initpara.period = kTimerPeriod;
    timer_initpara.clockdivision = TIMER_CKDIV_DIV1;
    timer_initpara.repetitioncounter = 0;

    timer_init(TIMERx, &timer_initpara);

    timer_flag_clear(TIMERx, UINT32_MAX);
    timer_interrupt_flag_clear(TIMERx, UINT32_MAX);

    timer_interrupt_enable(TIMERx, TIMER_INT_UP);

    NVIC_SetPriority(TIMERx_IRQn, (1UL << __NVIC_PRIO_BITS) - 1UL); // Lowest priority
    NVIC_EnableIRQ(TIMERx_IRQn);

#if defined(SOFTUART0_ENABLE_RX)
    rcu_periph_clock_enable(SOFTUART_RX_RCU_TIMERx);
    timer_deinit(SOFTUART_RX_TIMERx);
    timer_struct_para_init(&timer_initpara);

    timer_initpara.prescaler = 0;
    timer_initpara.alignedmode = TIMER_COUNTER_EDGE;
    timer_initpara.counterdirection = TIMER_COUNTER_UP;
    timer_initpara.period = kTimerPeriod / 2;
    timer_initpara.clockdivision = TIMER_CKDIV_DIV1;
    timer_initpara.repetitioncounter = 0;

    timer_init(SOFTUART_RX_TIMERx, &timer_initpara);

    timer_flag_clear(SOFTUART_RX_TIMERx, UINT32_MAX);
    timer_interrupt_flag_clear(SOFTUART_RX_TIMERx, UINT32_MAX);

    timer_interrupt_enable(SOFTUART_RX_TIMERx, TIMER_INT_UP);

    NVIC_SetPriority(SOFTUART_RX_TIMERx_IRQn, (1UL << __NVIC_PRIO_BITS) - 1UL); // Lowest priority
    NVIC_EnableIRQ(SOFTUART_RX_TIMERx_IRQn);
#endif // SOFTUART0_ENABLE_RX
}

bool PutCharTimer(int character) {
    const auto kChar = static_cast<uint8_t>(character);

    NVIC_DisableIRQ(TIMERx_IRQn);

    const uint32_t kCurrentHead = s_tx_buffer.head;
    const uint32_t kTail = s_tx_buffer.tail;
    const uint32_t kNextHead = (kCurrentHead + 1U) & (kBufferSize - 1U);

    if (kNextHead == kTail) {
        NVIC_EnableIRQ(TIMERx_IRQn);
        return false;
    }

    s_tx_buffer.buffer[kCurrentHead] = kChar;
    __COMPILER_BARRIER();
    s_tx_buffer.head = kNextHead;

    if (s_tx_state == TxState::kIdle) {
        s_tx_state = TxState::kStartBit;
        __COMPILER_BARRIER();
        timer_counter_value_config(TIMERx, 0);
        timer_enable(TIMERx);
    }

    NVIC_EnableIRQ(TIMERx_IRQn);
    return true;
}
} // namespace

void Init() {
    is_init = true;
    s_tx_state = TxState::kIdle;
    s_tx_buffer.head = 0;
    s_tx_buffer.tail = 0;
#if defined(SOFTUART0_ENABLE_RX)
    s_rx_state = RxState::kIdle;
    s_rx_buffer.head = 0;
    s_rx_buffer.tail = 0;
#endif // SOFTUART0_ENABLE_RX

    GpioConfig();
    TimersConfig();
}

void PutChar(int character) {
    if (!is_init) [[unlikely]] {
        return;
    }

    if (character == '\n') {
        while (!PutCharTimer('\r')) {
        }
    }

    while (!PutCharTimer(character)) {
    }
}

void Puts(const char* string) {
    while (*string != '\0') {
        PutChar(*string++);
    }

    PutChar('\n');
}

int GetChar() {
#if defined(SOFTUART0_ENABLE_RX)
    if (s_rx_buffer.head == s_rx_buffer.tail) [[likely]] {
        return -1;
    }

    const auto kChar = s_rx_buffer.buffer[s_rx_buffer.tail];
    s_rx_buffer.tail = (s_rx_buffer.tail + 1U) & (kBufferSize - 1U);

    return kChar;
#else
    return -1;
#endif // SOFTUART0_ENABLE_RX
}
} // namespace uart0
