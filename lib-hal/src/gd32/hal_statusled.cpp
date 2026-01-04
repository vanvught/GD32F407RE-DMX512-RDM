/**
 * @file hal_statusled.cpp
 *
 */
/* Copyright (C) 2025 by Arjan van Vught mailto:info@gd32-dmx.org
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

#if defined(DEBUG_HAL)
#undef NDEBUG
#endif

#include <cstdint>

#include "hal.h"
#include "hal_statusled.h"
#include "softwaretimers.h"
#include "gd32.h"
#include "firmware/debug/debug_debug.h"

static TimerHandle_t s_timer_id = kTimerIdNone;

#if !defined(HAL_HAVE_PORT_BIT_TOGGLE)
static int32_t s_toggle_led = 1;
#endif

static void Ledblink([[maybe_unused]] TimerHandle_t handle)
{
#if defined(HAL_HAVE_PORT_BIT_TOGGLE)
    GPIO_TG(LED_BLINK_GPIO_PORT) = LED_BLINK_PIN;
#else
    s_toggle_led = -s_toggle_led;

    if (s_toggle_led > 0)
    {
#if defined(CONFIG_LEDBLINK_USE_PANELLED)
        hal::PanelLedOn(hal::panelled::ACTIVITY);
#else
        GPIO_BOP(LED_BLINK_GPIO_PORT) = LED_BLINK_PIN;
#endif
    }
    else
    {
#if defined(CONFIG_LEDBLINK_USE_PANELLED)
        hal::PanelLedOff(hal::panelled::ACTIVITY);
#else
        GPIO_BC(LED_BLINK_GPIO_PORT) = LED_BLINK_PIN;
#endif
    }
#endif
}

namespace hal::statusled
{
void SetFrequency(uint32_t frequency_hz)
{
    DEBUG_ENTRY();
    DEBUG_PRINTF("s_timer_id=%d, frequency_hz=%u", s_timer_id, frequency_hz);

    if (s_timer_id == kTimerIdNone)
    {
        s_timer_id = SoftwareTimerAdd((1000U / frequency_hz), Ledblink);
        DEBUG_EXIT();
        return;
    }

    switch (frequency_hz)
    {
        case 0:
            SoftwareTimerDelete(s_timer_id);
#if defined(CONFIG_LEDBLINK_USE_PANELLED)
            hal::PanelLedOff(hal::panelled::ACTIVITY);
#else
            GPIO_BC(LED_BLINK_GPIO_PORT) = LED_BLINK_PIN;
#endif
            break;
#if !defined(CONFIG_HAL_USE_MINIMUM)
        case 1:
            SoftwareTimerChange(s_timer_id, (1000U / 1));
            break;
        case 3:
            SoftwareTimerChange(s_timer_id, (1000U / 3));
            break;
        case 5:
            SoftwareTimerChange(s_timer_id, (1000U / 5));
            break;
        case 8:
            SoftwareTimerChange(s_timer_id, (1000U / 8));
            break;
#endif
        case 255:
            SoftwareTimerDelete(s_timer_id);
#if defined(CONFIG_LEDBLINK_USE_PANELLED)
            hal::PanelLedOn(hal::panelled::ACTIVITY);
#else
            GPIO_BOP(LED_BLINK_GPIO_PORT) = LED_BLINK_PIN;
#endif
            break;
        default:
            SoftwareTimerChange(s_timer_id, (1000U / frequency_hz));
            break;
    }

    DEBUG_EXIT();
}
} // namespace hal::statusled
