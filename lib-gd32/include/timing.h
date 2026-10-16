/**
 * @file timing.h
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

#ifndef GD32_TIMING_H_
#define GD32_TIMING_H_

#include <cstdint>

#include "gd32_timers.h"

#ifdef CONFIG_TIME_USE_SYSTICK
extern volatile uint32_t gv_systick_millis;
#elif defined(USE_FREE_RTOS)
#include "FreeRTOS.h"
#include "task.h"
#endif // CONFIG_TIME_USE_SYSTICK

namespace timing {
// Use for:
// microsecond delays
// profiling
// short protocol timing
// busy waits
[[nodiscard]] inline uint32_t Micros() {
    return gd32::Micros();
}

inline void DelayUs(uint32_t micros, uint32_t offset_micros = 0) {
    gd32::DelayUs(micros, offset_micros);
}

// Use for:
// timeouts
// periodic scheduling
// millisecond-level elapsed time
// network polling
// UI timers
[[nodiscard]] inline uint32_t Millis() {
    return gd32::Millis();
}

[[nodiscard]] inline uint32_t UpTime() {
  return gd32::UpTime();
}
} // namespace timing

#endif // GD32_TIMING_H_
