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

#if defined(GD32H7XX)
#define TIMERx            TIMER15
#define RCU_TIMERx        RCU_TIMER15
#define TIMERx_IRQHandler TIMER15_IRQHandler
#define TIMERx_IRQn       TIMER15_IRQn
#else
#define TIMERx            TIMER9
#define RCU_TIMERx        RCU_TIMER9
#define TIMERx_IRQHandler TIMER0_UP_TIMER9_IRQHandler
#define TIMERx_IRQn       TIMER0_UP_TIMER9_IRQn
#endif

#if defined(GD32H7XX)
#define TIMER_CLOCK_FREQ  (AHB_CLOCK_FREQ)
#elif defined(GD32F4XX)
#define TIMER_CLOCK_FREQ  (APB2_CLOCK_FREQ * 2)
#else
#define TIMER_CLOCK_FREQ  (APB2_CLOCK_FREQ)
#endif

#if !defined(SOFTUART_TX_PINx)
#define SOFTUART_TX_PINx          GPIO_PIN_9
#define SOFTUART_TX_GPIOx         GPIOA
#define SOFTUART_TX_RCU_GPIOx     RCU_GPIOA
#endif

#if defined(SOFTUART0_ENABLE_RX)
#if !defined(SOFTUART_RX_PINx)
#define SOFTUART_RX_PINx          GPIO_PIN_10
#define SOFTUART_RX_GPIOx         GPIOA
#define SOFTUART_RX_RCU_GPIOx     RCU_GPIOA
#if defined(GD32H7XX)
#error
#else
#define SOFTUART_RX_TIMERx                  TIMER0
#define SOFTUART_RX_RCU_TIMERx              RCU_TIMER0
#define SOFTUART_RX_EXTIx_IRQHandler        EXTI10_15_IRQHandler
#define SOFTUART_RX_EXTIx_IRQn              EXTI10_15_IRQn
#define SOFTUART_RX_GPIO_PORT_SOURCE_GPIOx  GPIO_PORT_SOURCE_GPIOB
#define SOFTUART_RX_GPIO_PIN_SOURCE_x       GPIO_PIN_SOURCE_14
#endif
#endif // !defined(SOFTUART_RX_PINx)
static_assert(TIMERx != SOFTUART_RX_TIMERx);
#endif // defined (SOFTUART0_ENABLE_RX)

static constexpr uint32_t kBaudRate =
#if defined(SOFTUART0_ENABLE_RX)
    38400;
#else
    115200;
#endif // defined (SOFTUART0_ENABLE_RX)
static constexpr uint32_t kTimerPeriod = ((TIMER_CLOCK_FREQ / kBaudRate) - 1U);
static constexpr uint32_t kBufferSize = 128U;

enum class TxState { kIdle, kStartBit, kData, kStopBit };
enum class RxState { kIdle, kVerifyStart, kData, kStopBit };

struct CircularBuffer {
    uint8_t buffer[kBufferSize];
    uint32_t head;
    uint32_t tail;
};

static volatile CircularBuffer s_tx_buffer __attribute__((aligned(4)));
static volatile TxState s_tx_state;
static volatile uint8_t s_tx_data;
static volatile uint8_t s_tx_shift;

#if defined(SOFTUART0_ENABLE_RX)
static volatile CircularBuffer s_rx_buffer __attribute__((aligned(4)));
static volatile RxState s_rx_state;
#endif // defined(SOFTUART0_ENABLE_RX)

extern "C" {
void TIMERx_IRQHandler() {
    const auto kIntFlag = TIMER_INTF(TIMERx);

    if ((kIntFlag & TIMER_INT_FLAG_UP) == TIMER_INT_FLAG_UP) {
        switch (s_tx_state) {
            case TxState::kStartBit:
                GPIO_BC(SOFTUART_TX_GPIOx) = SOFTUART_TX_PINx;

                s_tx_state = TxState::kData;
                s_tx_data = s_tx_buffer.buffer[s_tx_buffer.tail];
                s_tx_buffer.tail = (s_tx_buffer.tail + 1) & (kBufferSize - 1);
                s_tx_shift = 0;
                break;
            case TxState::kData:
                if (s_tx_data & (1U << s_tx_shift)) {
                    GPIO_BOP(SOFTUART_TX_GPIOx) = SOFTUART_TX_PINx;
                } else {
                    GPIO_BC(SOFTUART_TX_GPIOx) = SOFTUART_TX_PINx;
                }

                s_tx_shift = s_tx_shift + 1;

                if (s_tx_shift == 8) {
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

    TIMER_INTF(TIMERx) = static_cast<uint32_t>(~kIntFlag);
}
#if defined(SOFTUART0_ENABLE_RX)
void SOFTUART_RX_EXTIx_IRQHandler() {
}
#endif
}

static bool PutCharTimer(int c) {
    const auto kChar = static_cast<uint8_t>(c);

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

namespace uart0 {
static auto is_init = false;

static void GpioConfig() {
#if defined(LED3_GPIO_PINx)
    rcu_periph_clock_enable(LED3_RCU_GPIOx);
#if defined(GPIO_INIT)
    gpio_init(LED3_GPIOx, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, LED3_GPIO_PINx);
#else
    gpio_mode_set(LED3_GPIOx, GPIO_MODE_OUTPUT, GPIO_PUPD_PULLDOWN, LED3_GPIO_PINx);
    gpio_output_options_set(LED3_GPIOx, GPIO_OTYPE_PP, GPIO_OSPEED, LED3_GPIO_PINx);
#endif // defined(GPIO_INIT)

    GPIO_BC(LED3_GPIOx) = LED3_GPIO_PINx;
#endif // defined(LED3_GPIO_PINx)

    rcu_periph_clock_enable(SOFTUART_TX_RCU_GPIOx);

#if defined(GPIO_INIT)
    gpio_init(SOFTUART_TX_GPIOx, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, SOFTUART_TX_PINx);
#else
    gpio_mode_set(SOFTUART_TX_GPIOx, GPIO_MODE_OUTPUT, GPIO_PUPD_PULLDOWN, SOFTUART_TX_PINx);
    gpio_output_options_set(SOFTUART_TX_GPIOx, GPIO_OTYPE_PP, GPIO_OSPEED, SOFTUART_TX_PINx);
#endif // defined(GPIO_INIT)

#if defined(SOFTUART0_ENABLE_RX)
#if defined(GPIO_INIT)
    gpio_init(SOFTUART_RX_GPIOx, GPIO_MODE_IPD, GPIO_OSPEED_50MHZ, SOFTUART_RX_PINx);
#else
#endif // defined(GPIO_INIT)
#endif // defined(SOFTUART0_ENABLE_RX)

    GPIO_BOP(SOFTUART_TX_GPIOx) = SOFTUART_TX_PINx;
}

static void TimersConfig() {
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

    timer_flag_clear(TIMERx, ~0);
    timer_interrupt_flag_clear(TIMERx, ~0);

    timer_interrupt_enable(TIMERx, TIMER_INT_UP);

    NVIC_SetPriority(TIMERx_IRQn, 2);
    NVIC_EnableIRQ(TIMERx_IRQn);

#if defined(SOFTUART0_ENABLE_RX)
    rcu_periph_clock_enable(SOFTUART_RX_RCU_TIMERx);
    timer_deinit(SOFTUART_RX_TIMERx);
    timer_struct_para_init(&timer_initpara);

    timer_initpara.prescaler = 0;
    timer_initpara.alignedmode = TIMER_COUNTER_EDGE;
    timer_initpara.counterdirection = TIMER_COUNTER_UP;
    timer_initpara.period = kTimerPeriod;
    timer_initpara.clockdivision = TIMER_CKDIV_DIV1;
    timer_initpara.repetitioncounter = 0;

    timer_init(SOFTUART_RX_TIMERx, &timer_initpara);
    timer_enable(SOFTUART_RX_TIMERx);
#endif
}

void Init() {
    is_init = true;
    s_tx_state = TxState::kIdle;
    s_tx_buffer.head = 0;
    s_tx_buffer.tail = 0;
#if defined(SOFTUART0_ENABLE_RX)
    s_rx_state = RxState::kIdle;
    s_rx_buffer.head = 0;
    s_rx_buffer.tail = 0;
#endif // defined(SOFTUART0_ENABLE_RX)

    GpioConfig();
    TimersConfig();
}

void PutChar(int c) {
    if (!is_init) [[unlikely]] {
        return;
    }

#if defined(LED3_GPIO_PINx)
    GPIO_BOP(LED3_GPIOx) = LED3_GPIO_PINx;
#endif

    if (c == '\n') {
        while (!PutCharTimer('\r')) {
        }
    }

    while (!PutCharTimer(c)) {
    }

#if defined(LED3_GPIO_PINx)
    GPIO_BC(LED3_GPIOx) = LED3_GPIO_PINx;
#endif
}

void Puts(const char* s) {
    while (*s != '\0') {
        PutChar(*s++);
    }

    PutChar('\n');
}

int GetChar() {
#if defined(SOFTUART0_ENABLE_RX)
    if (s_rx_buffer.head == s_rx_buffer.tail) {
        return -1;
    }

    const uint8_t kC = s_rx_buffer.buffer[s_rx_buffer.tail];
    s_rx_buffer.tail = (s_rx_buffer.tail + 1U) & (kBufferSize - 1U);

    return kC;
#else
    return -1;
#endif // defined(SOFTUART0_ENABLE_RX)
}
} // namespace uart0
