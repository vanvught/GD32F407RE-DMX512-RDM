/**
 * @file hal_reboot.cpp
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

#if defined(DEBUG_HAL)
#undef NDEBUG
#endif

#include <cstdio>

#include "gd32.h"
#include "hal.h"
#include "hal_statusled.h"

#if !defined(DISABLE_RTC)
#include "hwclock.h"
#endif

#include "configstore.h"

#if !defined(NO_EMAC)
namespace network
{
void Shutdown();
} // namespace network
#endif

namespace hal
{
void RebootHandler();

bool Reboot()
{
    puts("Rebooting ...");

    fwdgt_config(0xFFFF, FWDGT_PSC_DIV64);

    ConfigstoreCommit();
#if !defined(DISABLE_RTC)
    HwClock::Get()->SysToHc();
#endif
    hal::RebootHandler();
#if !defined(NO_EMAC)
    network::Shutdown();
#endif
    hal::statusled::SetMode(hal::statusled::Mode::OFF_OFF);

    NVIC_SystemReset();

    __builtin_unreachable();
    return true;
}
} // namespace hal
