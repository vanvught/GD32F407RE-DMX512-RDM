/**
 * @file gd32_timers.cpp
 *
 */
/* Copyright (C) 2026 by Arjan van Vught mailto:info@gd32-dmx.org
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

#if defined(CONFIG_TIME_USE_SYSTICK) && defined(CONFIG_TIME_USE_TIMER)
#error
#endif // defined(CONFIG_TIME_USE_SYSTICK) && defined(CONFIG_TIME_USE_TIMER)

#include <cstdint>
#include <cassert>

#include "gd32_timers.h"
#include "common/utils/utils_units.h"
#include "gd32_debug.h"
#include "gd32.h" // IWYU pragma: keep

#if defined(CONFIG_TIME_USE_SYSTICK)
volatile uint32_t gv_systick_millis;

extern "C" void SysTick_Handler() {
    gv_systick_millis = gv_systick_millis + 1;
}
#endif // CONFIG_TIME_USE_SYSTICK

struct HwTimersSeconds gv_seconds;

#if defined(CONFIG_TIMER6_HAVE_NO_IRQ_HANDLER)
extern "C" void TIMER6_IRQHandler() {
    const auto kIntFlag = TIMER_INTF(TIMER6);

    if ((kIntFlag & TIMER_INT_FLAG_UP) == TIMER_INT_FLAG_UP) {
        gv_seconds.uptime = gv_seconds.uptime + 1;
    }

    TIMER_INTF(TIMER6) = ~kIntFlag;
}
#endif // CONFIG_TIMER6_HAVE_NO_IRQ_HANDLER

namespace gd32::timers {
void Start() {
    GD32_TIMERS_DEBUG_ENTRY();

#if defined(GD32H7XX)
#elif defined(GD32F4XX)
    // AHB = SYSCLK = 240 MHz (GD32F470), others = 200 MHz
    // APB1 = AHB / 4 =   50 MHz => APB1PSC = 0b101
    // APB2 = AHB / 2  = 100 MHz => APB2PSC = 0b100
    rcu_timer_clock_prescaler_config(RCU_TIMER_PSC_MUL4);

    // If APB1PSC/APB2PSC in RCU_CFG0 register is 0b0xx(CK_APBx = CK_AHB),
    // 0b100(CK_APBx = CK_AHB/2), or 0b101(CK_APBx = CK_AHB/4), the TIMER
    // clock is equal to CK_AHB(CK_TIMERx = CK_AHB).

    // TIMER in APB1 domain: CK_TIMERx = AHB = 200 MHz => 240 MHz (GD32F470).
    // TIMER in APB2 domain: CK_TIMERx = AHB = 200 MHz => 240 MHz (GD32F470).
#else
#endif // GD32H7XX
    timer5::Start();
    timer6::Start();
#if defined(CONFIG_TIME_USE_SYSTICK)
    systick::Start();
#endif // CONFIG_TIME_USE_SYSTICK
#if defined(CONFIG_TIME_USE_TIMER)
    timer_time::Start();
#endif // CONFIG_TIME_USE_TIMER
    dwt::Start();

    GD32_TIMERS_DEBUG_EXIT();
}

namespace dwt {
static constexpr auto kTicksPerUs = (MCU_CLOCK_FREQ / 1000000U);

void Start() {
    GD32_TIMERS_DEBUG_ENTRY();

    assert(MCU_CLOCK_FREQ == SystemCoreClock);

    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;

    GD32_TIMERS_DEBUG_EXIT();
}

#pragma GCC push_options
#pragma GCC optimize("O2")

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
void DelayUs(uint32_t micros, uint32_t offset_micros) {
    const auto kTicks = micros * kTicksPerUs;

    uint32_t ticks_count = 0;
    uint32_t ticks_previous;

    if (offset_micros == 0) {
        ticks_previous = DWT->CYCCNT;
    } else {
        ticks_previous = offset_micros;
    }

    while (true) {
        const auto kTicksNow = DWT->CYCCNT;

        if (kTicksNow != ticks_previous) {
            if (kTicksNow > ticks_previous) {
                ticks_count += kTicksNow - ticks_previous;
            } else {
                ticks_count += UINT32_MAX - ticks_previous + kTicksNow;
            }

            if (ticks_count >= kTicks) {
                break;
            }

            ticks_previous = kTicksNow;
        }
    }
}

uint32_t Micros() {
    static uint32_t cycles_previous;
    static uint32_t micros_accumulated;
    static uint32_t cycle_remainder;

    const uint32_t kCyclesNow = DWT->CYCCNT;
    const uint32_t kDeltaCycles = kCyclesNow - cycles_previous;
    cycles_previous = kCyclesNow;

    const uint32_t kTotal = cycle_remainder + kDeltaCycles;

    micros_accumulated += kTotal / kTicksPerUs;
    cycle_remainder = kTotal % kTicksPerUs;

    return micros_accumulated;
}

#pragma GCC pop_options
} // namespace dwt

#if defined(CONFIG_TIME_USE_SYSTICK)
namespace systick {
void Start() {
    GD32_TIMERS_DEBUG_ENTRY();
    // Setup systick timer for 1000Hz interrupts
    if (SysTick_Config(SystemCoreClock / 1000U)) {
        while (true) {
        }
    }

    gv_systick_millis = 0;

    NVIC_SetPriority(SysTick_IRQn, (1UL << __NVIC_PRIO_BITS) - 1UL); // Lowest priority

    GD32_TIMERS_DEBUG_EXIT();
}
} // namespace systick
#endif // CONFIG_TIME_USE_SYSTICK

namespace timer5 {
void Start() {
    GD32_TIMERS_DEBUG_ENTRY();

    rcu_periph_clock_enable(RCU_TIMER5);

    timer_deinit(TIMER5);

    timer_parameter_struct timer_initpara;
    timer_struct_para_init(&timer_initpara);

    timer_initpara.prescaler = TIMER_PSC_1MHZ;
    timer_initpara.period = UINT32_MAX;
    timer_init(TIMER5, &timer_initpara);
    timer_enable(TIMER5);

    GD32_TIMERS_DEBUG_EXIT();
}

void Stop() {
    GD32_TIMERS_DEBUG_ENTRY();

    timer_disable(TIMER5);
    rcu_periph_clock_disable(RCU_TIMER5);

    GD32_TIMERS_DEBUG_EXIT();
}

void Delay1ms() {
    TIMER_CNT(TIMER5) = 0;
    do {
        __DMB();
    } while (TIMER_CNT(TIMER5) < common::units::kUsPerMs);
}
} // namespace timer5

namespace timer6 {
void Start() {
    GD32_TIMERS_DEBUG_ENTRY();

    gv_seconds.uptime = 0;

    rcu_periph_clock_enable(RCU_TIMER6);
    timer_deinit(TIMER6);

    timer_parameter_struct timer_initpara;
    timer_struct_para_init(&timer_initpara);

    timer_initpara.prescaler = TIMER_PSC_10KHZ;
    timer_initpara.period = (10000 - 1); // 1 second
    timer_init(TIMER6, &timer_initpara);

    timer_counter_value_config(TIMER6, 0);

    timer_interrupt_flag_clear(TIMER6, UINT32_MAX);
    timer_interrupt_enable(TIMER6, TIMER_INT_UP);

    NVIC_SetPriority(TIMER6_IRQn, (1UL << __NVIC_PRIO_BITS) - 1UL); // Lowest priority
    NVIC_EnableIRQ(TIMER6_IRQn);

    timer_enable(TIMER6);

    GD32_TIMERS_DEBUG_EXIT();
}

#pragma GCC push_options
#pragma GCC optimize("O2")

uint32_t Millis() {
    auto seconds = gv_seconds.uptime;
    auto timer_cnt = TIMER_CNT(TIMER6);

    if ((TIMER_INTF(TIMER6) & TIMER_INT_FLAG_UP) != 0U) {
        seconds++;
        timer_cnt = TIMER_CNT(TIMER6);
    }

    return (seconds * common::units::kMsPerSecond) + (timer_cnt / 10U);
}

#pragma GCC pop_options
} // namespace timer6
} // namespace gd32::timers
