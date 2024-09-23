/**
 * @file timer6.cpp
 *
 */
/* Copyright (C) 2024 by Arjan van Vught mailto:info@gd32-dmx.org
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
#pragma GCC optimize ("O2")

#include "gd32.h"

struct HwTimersSeconds g_Seconds;

extern "C" {
#if defined (CONFIG_TIMER6_HAVE_NO_IRQ_HANDLER)
void TIMER6_IRQHandler() {
	const auto nIntFlag = TIMER_INTF(TIMER6);

	if ((nIntFlag & TIMER_INT_FLAG_UP) == TIMER_INT_FLAG_UP) {
		g_Seconds.nUptime++;
	}

	TIMER_INTF(TIMER6) = static_cast<uint32_t>(~nIntFlag);
}
#endif
}

void timer6_config() {
	g_Seconds.nUptime = 0;

	rcu_periph_clock_enable(RCU_TIMER6);
	timer_deinit(TIMER6);

	timer_parameter_struct timer_initpara;
	timer_struct_para_init(&timer_initpara);

	timer_initpara.prescaler = TIMER_PSC_10KHZ;
	timer_initpara.period = (10000 - 1);		// 1 second
	timer_init(TIMER6, &timer_initpara);

	timer_counter_value_config(TIMER6, 0);

	timer_interrupt_flag_clear(TIMER6, ~0);

	timer_interrupt_enable(TIMER6, TIMER_INT_UP);

	NVIC_SetPriority(TIMER6_IRQn, (1UL<<__NVIC_PRIO_BITS)-1UL); // Lowest priority
	NVIC_EnableIRQ(TIMER6_IRQn);

	timer_enable(TIMER6);
}

uint32_t timer6_get_elapsed_milliseconds() {
    const auto nUptimeFirst = g_Seconds.nUptime;
    auto nTimerCount = TIMER_CNT(TIMER6);
    const auto nUptimeSecond = g_Seconds.nUptime;

    // Check for consistency
    if (__builtin_expect((nUptimeFirst == nUptimeSecond), 1)) {
        // No overflow detected, return the calculated time
        return (nUptimeFirst * 1000U) + (nTimerCount / 10U);
    }

    // Potential overflow detected, re-read the timer count
    nTimerCount = TIMER_CNT(TIMER6);
    return (nUptimeSecond * 1000U) + (nTimerCount / 10U);
}
