/**
 * @file gd32_timers.h
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

#ifndef GD32_TIMERS_H_
#define GD32_TIMERS_H_

#if defined(USE_FREE_RTOS) && defined(CONFIG_TIME_USE_SYSTICK)
#error
#endif

#include <cstdint>

#if defined(USE_FREE_RTOS)
#include "FreeRTOS.h"
#include "task.h"
#endif // USE_FREE_RTOS

#if defined(CONFIG_TIME_USE_SYSTICK)
extern volatile uint32_t gv_systick_millis;
#endif // CONFIG_TIME_USE_SYSTICK

struct HwTimersSeconds {
#if !defined(CONFIG_NET_ENABLE_PTP)
    volatile uint32_t timeval;
#endif // CONFIG_NET_ENABLE_PTP
    volatile uint32_t uptime;
};

extern struct HwTimersSeconds gv_seconds;

namespace gd32 {
namespace timers {
void Start();
namespace dwt {
void Start();
void DelayUs(uint32_t micros, uint32_t offset_micros = 0);
[[nodiscard]] uint32_t Micros();
} // namespace dwt
namespace systick {
void Start();
} // namespace systick
namespace timer5 {
void Start();
void Stop();
void Delay1ms();
} // namespace timer5
namespace timer6 {
void Start();
[[nodiscard]] uint32_t Millis();
} // namespace timer6
namespace timer_time {
void Start();
} // namespace timer_time
} // namespace timers

[[nodiscard]] inline uint32_t Micros() {
    return gd32::timers::dwt::Micros();
}

inline void DelayUs(uint32_t micros, uint32_t offset_micros = 0) {
    gd32::timers::dwt::DelayUs(micros, offset_micros);
}

[[nodiscard]] inline uint32_t Millis() {
#if defined(CONFIG_TIME_USE_SYSTICK)
    return gv_systick_millis;
#elif defined(USE_FREE_RTOS)
    return xTaskGetTickCount();
#else
    return gd32::timers::timer6::Millis();
#endif // CONFIG_TIME_USE_SYSTICK
}

[[nodiscard]] inline uint32_t UpTime() {
    return gv_seconds.uptime;
}
} // namespace gd32

#endif // GD32_TIMERS_H_
