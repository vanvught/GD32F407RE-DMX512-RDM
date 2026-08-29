/**
 * @file board_statusled.cpp
 *
 */
/* Copyright (C) 2025-2026 by Arjan van Vught mailto:info@gd32-dmx.org
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

#include "board_statusled.h"
#include "common/utils/utils_units.h"
#include "softwaretimers.h"
#include "board_debug.h"
#include "gd32.h" // IWYU pragma: keep

namespace {
TimerHandle_t s_timer_id = kTimerIdNone;

#if !defined(MCU_HAVE_GPIO_TG)
int32_t s_toggle_led = 1;
#endif

void Ledblink([[maybe_unused]] TimerHandle_t handle) {
#if defined(MCU_HAVE_GPIO_TG)
    GPIO_TG(LED_BLINK_GPIO_PORT) = LED_BLINK_PIN;
#else
    s_toggle_led = -s_toggle_led;

    if (s_toggle_led > 0) {
        GPIO_BOP(LED_BLINK_GPIO_PORT) = LED_BLINK_PIN;
    } else {
        GPIO_BC(LED_BLINK_GPIO_PORT) = LED_BLINK_PIN;
    }
#endif
}
} // namespace

namespace board::statusled {
void SetFrequency(uint32_t frequency_hz) {
    BOARD_DEBUG_ENTRY();
    BOARD_DEBUG_PRINTF("s_timer_id=%d, frequency_hz=%u", static_cast<int>(s_timer_id), static_cast<unsigned>(frequency_hz));

    if (s_timer_id == kTimerIdNone) {
        s_timer_id = SoftwareTimerAdd((common::units::kMsPerSecond / frequency_hz), Ledblink);
        BOARD_DEBUG_EXIT();
        return;
    }

    switch (frequency_hz) {
        case 0:
            SoftwareTimerDelete(s_timer_id);

            GPIO_BC(LED_BLINK_GPIO_PORT) = LED_BLINK_PIN;
            break;
#if !defined(CONFIG_HAL_USE_MINIMUM)
        case 1:
            SoftwareTimerChange(s_timer_id, (common::units::kMsPerSecond / 1));
            break;
        case 3:
            SoftwareTimerChange(s_timer_id, (common::units::kMsPerSecond / 3));
            break;
        case 5:
            SoftwareTimerChange(s_timer_id, (common::units::kMsPerSecond / 5));
            break;
        case 8:
            SoftwareTimerChange(s_timer_id, (common::units::kMsPerSecond / 8));
            break;
#endif
        case 255:
            SoftwareTimerDelete(s_timer_id);
            GPIO_BOP(LED_BLINK_GPIO_PORT) = LED_BLINK_PIN;
            break;
        default:
            SoftwareTimerChange(s_timer_id, (common::units::kMsPerSecond / frequency_hz));
            break;
    }

    BOARD_DEBUG_EXIT();
}
} // namespace board::statusled
