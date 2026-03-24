/**
 * @file hal.h
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

#ifndef GD32_HAL_H_
#define GD32_HAL_H_

#include <cstdint>

#include "gd32.h" // IWYU pragma: keep

#if defined(ENABLE_USB_HOST)
extern "C"
{
#include "usbh_core.h"
    extern usbh_host usb_host;
}
#endif

#if defined(DEBUG_STACK)
void stack_debug_run();
#endif
#if defined(DEBUG_EMAC)
void emac_debug_run();
#endif

#if defined(USE_FREE_RTOS)
#include "FreeRTOS.h"
#include "task.h"
#endif

#include "softwaretimers.h" // IWYU pragma: keep

#if !defined(DISABLE_RTC)
#include "hwclock.h" // IWYU pragma: keep
#endif

#include "hal_panelled.h"

#if defined(CONFIG_HAL_USE_SYSTICK)
extern volatile uint32_t gv_nSysTickMillis;
#endif

namespace hal
{
inline constexpr const char kWebsite[] = "https://gd32-dmx.org";
inline constexpr float kCoreTemperatureMin = -40.0;
inline constexpr float kCoreTemperatureMax = +85.0;

inline void Run()
{
#if defined(ENABLE_USB_HOST)
    usbh_core_task(&usb_host);
#endif
#if !defined(USE_FREE_RTOS)
    SoftwareTimerRun();
#endif
    hal::panelled::Run();
#if defined(DEBUG_STACK)
    stack_debug_run();
#endif
#if defined(DEBUG_EMAC)
    emac_debug_run();
#endif
}
} // namespace hal

#endif // GD32_HAL_H_
