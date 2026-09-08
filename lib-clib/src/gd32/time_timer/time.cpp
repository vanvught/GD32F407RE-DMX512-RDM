/**
 * @file time.cpp
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

#if !defined(CONFIG_TIME_USE_TIMER)
#error
#endif // CONFIG_TIME_USE_TIMER

#pragma GCC push_options
#pragma GCC optimize("O2")

#include <ctime>
#include <sys/time.h>
#include <cstdint>
#include <cassert>

#include "gd32.h" // IWYU pragma: keep
#include "gd32_timers.h"
#include "gd32_debug.h"

// H7xx  		-> TIMER16
// F10x/F30x	-> TIMER0
// other 		-> TIMER7 / TIMER7_UP_TIMER12_IRQn

#if defined(GD32H7XX)
  #define TIMERx          		TIMER16
  #define RCU_TIMERx      		RCU_TIMER16
  #define TIMERx_IRQn     		TIMER16_IRQn
  #define TIMERx_IRQ_HANDLER	TIMER16_IRQHandler
#else
  #if defined(GD32F10X) || defined(GD32F30X) // TIMER7 does not exist on GD32F107
    #define TIMERx        		TIMER0
    #define RCU_TIMERx    		RCU_TIMER0
    #define TIMERx_IRQn   		TIMER0_UP_IRQn
	#define TIMERx_IRQ_HANDLER	TIMER0_UP_IRQHandler
  #else
    #define TIMERx        		TIMER7
    #define RCU_TIMERx    		RCU_TIMER7
    #define TIMERx_IRQn 		TIMER7_UP_TIMER12_IRQn
	#define TIMERx_IRQ_HANDLER	TIMER7_UP_TIMER12_IRQHandler
  #endif // defined(GD32F10X) || defined(GD32F30X)
#endif // GD32H7XX

#if defined(CONFIG_TIME_USE_TIMER) // Include IRQ handler when used only
extern "C" void TIMERx_IRQ_HANDLER() {
    const auto kIntFlag = TIMER_INTF(TIMERx);

    if ((kIntFlag & TIMER_INT_FLAG_UP) == TIMER_INT_FLAG_UP) {
        gv_seconds.timeval = gv_seconds.timeval + 1;
    }

    TIMER_INTF(TIMERx) = ~kIntFlag;
}
#endif // CONFIG_TIME_USE_TIMER

namespace gd32::timers::timer_time {
void Start() {
    GD32_TIMERS_DEBUG_ENTRY();

    gv_seconds.timeval = 0;

    rcu_periph_clock_enable(RCU_TIMERx);
    timer_deinit(TIMERx);

    timer_parameter_struct timer_initpara;
    timer_struct_para_init(&timer_initpara);

    timer_initpara.prescaler = TIMER_PSC_10KHZ;
    timer_initpara.alignedmode = TIMER_COUNTER_EDGE;
    timer_initpara.counterdirection = TIMER_COUNTER_UP;
    timer_initpara.period = (10000 - 1); // 1 second
	timer_initpara.clockdivision = TIMER_CKDIV_DIV1;
	timer_initpara.repetitioncounter = 0;
    timer_init(TIMERx, &timer_initpara);

    timer_interrupt_flag_clear(TIMERx, UINT32_MAX);

    timer_interrupt_enable(TIMERx, TIMER_INT_UP);

    NVIC_SetPriority(TIMERx_IRQn, (1UL << __NVIC_PRIO_BITS) - 1UL); // Lowest priority
    NVIC_EnableIRQ(TIMERx_IRQn);

    timer_enable(TIMERx);

	GD32_TIMERS_DEBUG_EXIT();
}
} // namespace gd32::timers::timer_time

extern "C" {
// number of seconds and microseconds since the Epoch,
//     1970-01-01 00:00:00 +0000 (UTC).
int gettimeofday(struct timeval* time_val, [[maybe_unused]] struct timezone* time_zone) { // NOLINT
    assert(tv != nullptr);

#if __CORTEX_M == 7
    __DMB();
#endif // __CORTEX_M == 7

    time_val->tv_sec = static_cast<time_t>(gv_seconds.timeval);
    time_val->tv_usec = static_cast<time_t>(TIMER_CNT(TIMERx) * 100U);

#if __CORTEX_M == 7
    __ISB();
#endif // __CORTEX_M == 7

    return 0;
}

int settimeofday(const struct timeval* time_val, [[maybe_unused]] const struct timezone* time_zone) { // NOLINT
    assert(tv != nullptr);

    // Disable the timer interrupt to prevent it from triggering while we adjust the counter
    TIMER_DMAINTEN(TIMERx) &= (~TIMER_INT_UP);
    TIMER_CTL0(TIMERx) &= (~TIMER_CTL0_CEN);

    gv_seconds.timeval = static_cast<uint32_t>(time_val->tv_sec);
    TIMER_CNT(TIMERx) = (static_cast<uint32_t>(time_val->tv_usec) / 100U) % 10000U;

    TIMER_INTF(TIMERx) = UINT32_MAX;
    TIMER_DMAINTEN(TIMERx) |= TIMER_INT_UP;
    TIMER_CTL0(TIMERx) |= TIMER_CTL0_CEN;

    return 0;
}

// time() returns the time as the number of seconds since the Epoch,
//      1970-01-01 00:00:00 +0000 (UTC).
time_t time(time_t* __timer) { // NOLINT
    struct timeval time_val;
    gettimeofday(&time_val, nullptr);

    if (__timer != nullptr) {
        *__timer = time_val.tv_sec;
    }

    return time_val.tv_sec;
}
}
