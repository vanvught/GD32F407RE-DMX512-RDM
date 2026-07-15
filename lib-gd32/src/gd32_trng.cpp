#if defined(GD32F20X) || defined(GD32F4XX)
/**
 * @file gd32_trng.h
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

#if defined(DEBUG_GD32_TRNG)
#undef NDEBUG
#endif

#include <cstdint>
#include <cstdio>

#include "gd32_trng.h"
#include "gd32.h" // IWYU pragma: keep
#include "firmware/debug/debug_debug.h"

namespace gd32::trng {
static constexpr uint32_t kTimeout = 0xFFFF;

static bool ReadyCheck() {
    uint32_t timeout = 0;
    FlagStatus trng_flag = RESET;

    // check wherther the random data is valid
    do {
        timeout++;
        trng_flag = trng_flag_get(TRNG_FLAG_DRDY);
    } while ((RESET == trng_flag) && (kTimeout > timeout));

    if (RESET == trng_flag) [[unlikely]] {
        // ready check timeout
        printf("Error: TRNG can't ready \r\n");
        trng_flag = trng_flag_get(TRNG_FLAG_CECS);
        printf("Clock error current status: %d \r\n", trng_flag);
        trng_flag = trng_flag_get(TRNG_FLAG_SECS);
        printf("Seed error current status: %d \r\n", trng_flag);
        return false;
    }

    return true;
}

bool Init() {
    DEBUG_ENTRY();

    // TRNG module clock enable
    rcu_periph_clock_enable(RCU_TRNG);
    // TRNG registers reset
    trng_deinit();
    trng_enable();
    // check TRNG work status
    const auto kReturn= ReadyCheck();

    DEBUG_PRINTF("%s", kReturn ? "Success" : "Error");
    DEBUG_EXIT();
    return kReturn;
}

static uint32_t out_last = 0;

bool Get(uint32_t& out) {
    if (ReadyCheck()) {
        out = trng_get_true_random_data();
        if (out != out_last) {
            out_last = out;
            return true;
        }
        // the random data is invalid
        puts("Error: Get the random data is same");
    }

    return false;
}
} // namespace gd32::trng

#endif