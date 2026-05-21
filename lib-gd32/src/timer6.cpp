/**
 * @file timer6.cpp
 *
 */
/* Copyright (C) 2024-2026 by Arjan van Vught mailto:info@gd32-dmx.org
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

#pragma GCC push_options
#pragma GCC optimize("O2")

#include <cstdint>

#include "gd32.h"

struct HwTimersSeconds gv_seconds;

extern "C" {
#if defined(CONFIG_TIMER6_HAVE_NO_IRQ_HANDLER)
void TIMER6_IRQHandler() {
    const auto kIntFlag = TIMER_INTF(TIMER6);

    if ((kIntFlag & TIMER_INT_FLAG_UP) == TIMER_INT_FLAG_UP) {
        gv_seconds.uptime = gv_seconds.uptime + 1;
    }

    TIMER_INTF(TIMER6) = static_cast<uint32_t>(~kIntFlag);
}
#endif
}

void Timer6Config() {
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
}

// Use for:
// timeouts
// periodic scheduling
// millisecond-level elapsed time
// network polling
// UI timers
uint32_t Timer6GetElapsedMilliseconds() {
    auto seconds = gv_seconds.uptime;
    auto timer_cnt = TIMER_CNT(TIMER6);

    if ((TIMER_INTF(TIMER6) & TIMER_INT_FLAG_UP) != 0U) {
        seconds++;
        timer_cnt = TIMER_CNT(TIMER6);
    }

    return seconds * 1000U + timer_cnt / 10U;
}

#pragma GCC pop_options
