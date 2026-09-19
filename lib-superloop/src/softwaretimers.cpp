/**
 * @file softwaretimers.cpp
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

#if defined(__GNUC__) && !defined(__clang__)
#if !defined(CONFIG_REMOTECONFIG_MINIMUM)
#pragma GCC optimize("O3")
#pragma GCC optimize("no-tree-loop-distribute-patterns")
#pragma GCC optimize("-funroll-loops")
#endif
#endif

#include <cstdint>
#include <cstdio>

#include "softwaretimers.h"
#include "timing.h" // IWYU pragma: keep
#include "firmware/ansi_colour.h"
#include "common/utils/utils_print.h"
#include "firmware/debug/debug_debug.h"

#ifdef DEBUG_SUPERLOOP_TIMERS
#define SUPERLOOP_TIMERS_DEBUG_ENTRY() DEBUG_ENTRY()
#define SUPERLOOP_TIMERS_DEBUG_EXIT() DEBUG_EXIT()
#define SUPERLOOP_TIMERS_DEBUG_PRINTF(...) DEBUG_PRINTF(__VA_ARGS__)
#define SUPERLOOP_TIMERS_DEBUG_PUTS(...) DEBUG_PUTS(__VA_ARGS__)
#else
#define SUPERLOOP_TIMERS_DEBUG_ENTRY() \
    do {                               \
    } while (false)
#define SUPERLOOP_TIMERS_DEBUG_EXIT() \
    do {                              \
    } while (false)
#define SUPERLOOP_TIMERS_DEBUG_PRINTF(...) \
    do {                                   \
    } while (false)
#define SUPERLOOP_TIMERS_DEBUG_PUTS(...) \
    do {                                 \
    } while (false)
#endif

namespace {
void Error(const char* func, const char* string, TimerHandle_t handle) {
    printf("%s%s: %s -> %d%s\n", ansi::Colours::Fg::kRed, func, string, static_cast<int>(handle), ansi::Colours::Fg::kDefault);
}

struct Timer {
    uint32_t expire_time;                      ///< Absolute expire time in milliseconds (wrap-around safe).
    uint32_t interval_millis;                  ///< Period in milliseconds
    int32_t id;                                ///< Opaque handle returned to the caller.
    TimerCallbackFunction_t callback_function; ///< Callback invoked on expiry; must be non-null.
};

Timer s_timers[kSoftwareTimersMax]; ///< Timer storage pool.
uint32_t s_timers_count = 0;        ///< Number of active timers (0..hal::SOFTWARE_TIMERS_MAX).
int32_t s_next_id = 0;              ///< Monotonically increasing ID source (may wrap, see notes).
uint32_t s_timer_current = 0;       ///< bRound-robin cursor for SoftwareTimerRun().
} // namespace

TimerHandle_t SoftwareTimerAdd(uint32_t interval_millis, TimerCallbackFunction_t k_callback_function) {
    SUPERLOOP_TIMERS_DEBUG_ENTRY();
    SUPERLOOP_TIMERS_DEBUG_PRINTF("s_timers_count=%u", static_cast<unsigned>(s_timers_count));

    if (s_timers_count >= kSoftwareTimersMax) {
        ERROR("Max timer limit reached");
        return -1;
    }

    const auto kCurrentTime = timing::Millis();
    // TODO (a) Prevent potential overflow when calculating expiration time.

    Timer new_timer = {
        .expire_time = kCurrentTime + interval_millis,
        .interval_millis = interval_millis,
        .id = s_next_id++,
        .callback_function = k_callback_function,
    };

    s_timers[s_timers_count++] = new_timer;

    SUPERLOOP_TIMERS_DEBUG_PRINTF("handle=%d", static_cast<int>(new_timer.id));
    SUPERLOOP_TIMERS_DEBUG_EXIT();
    return new_timer.id;
}

bool SoftwareTimerDelete(TimerHandle_t& handle) {
    SUPERLOOP_TIMERS_DEBUG_ENTRY();
    SUPERLOOP_TIMERS_DEBUG_PRINTF("handle=%d, s_timers_count=%u", static_cast<signed>(handle), static_cast<unsigned>(s_timers_count));

    for (uint32_t i = 0; i < s_timers_count; ++i) {
        if (s_timers[i].id == handle) {
            // Swap with the last timer to efficiently remove the current timer
            s_timers[i] = s_timers[s_timers_count - 1];
            --s_timers_count;
            // Keep the round-robin cursor within bounds.
            if (s_timer_current >= s_timers_count) {
                s_timer_current = 0;
            }

            handle = -1;

            SUPERLOOP_TIMERS_DEBUG_EXIT();
            return true;
        }
    }

    Error(__func__, "Timer not found", handle);

    SUPERLOOP_TIMERS_DEBUG_EXIT();
    return false;
}

bool SoftwareTimerChange(TimerHandle_t handle, uint32_t interval_millis) {
    SUPERLOOP_TIMERS_DEBUG_ENTRY();
    SUPERLOOP_TIMERS_DEBUG_PRINTF("handle=%d, s_timers_count=%u", static_cast<signed>(handle), static_cast<unsigned>(s_timers_count));

    for (uint32_t i = 0; i < s_timers_count; ++i) {
        if (s_timers[i].id == handle) {
            const auto kCurrentTime = timing::Millis();
            s_timers[i].expire_time = kCurrentTime + interval_millis;
            s_timers[i].interval_millis = interval_millis;
            return true;
        }
    }

    Error(__func__, "Timer not found", handle);

    SUPERLOOP_TIMERS_DEBUG_EXIT();
    return false;
}

void SoftwareTimerRun() {
    if (s_timers_count == 0) [[unlikely]] {
        return;
    }

    const uint32_t kNow = timing::Millis();
    Timer& timer = s_timers[s_timer_current];

    if (static_cast<int32_t>(kNow - timer.expire_time) >= 0) [[unlikely]] {
        const int32_t kId = timer.id;
        const uint32_t kInterval = timer.interval_millis;
        auto callback_function = timer.callback_function;

        callback_function(kId);

        // reschedule from NOW to avoid pile-ups after delays
        s_timers[s_timer_current].expire_time = kNow + kInterval;
    }

    // Advance round-robin cursor (bounded next call).
    ++s_timer_current;
    if (s_timer_current >= s_timers_count) {
        s_timer_current = 0;
    }
}
