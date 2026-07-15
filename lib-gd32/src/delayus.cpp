/**
 * @file delayus.cpp
 *
 */
/* Copyright (C) 2021-2026 by Arjan van Vught mailto:info@gd32-dmx.org
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

#if defined(DEBUG_UDELAY)
#undef NDEBUG
#endif

#include <cstdint>
#include <cassert>

#include "gd32.h" // IWYU pragma: keep

static constexpr auto kTicksPerUs = (MCU_CLOCK_FREQ / 1000000U);

void UdelayInit() {
    assert(MCU_CLOCK_FREQ == SystemCoreClock);

    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

namespace timing {
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
} // namespace timing

// Use for:
// microsecond delays
// profiling
// short protocol timing
// busy waits
uint32_t Gd32Micros() {
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