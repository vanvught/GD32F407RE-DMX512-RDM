#if !defined(ENABLE_TFTP_SERVER)
/**
 * @file remoteconfig.cpp
 *
 */
/* Copyright (C) 2022-2025 by Arjan van Vught mailto:info@gd32-dmx.org
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

#include "remoteconfig.h"
#include "display.h"
#include "gd32.h"
#include "firmware/debug/debug_debug.h"

void RemoteConfig::PlatformHandleTftpSet()
{
    DEBUG_ENTRY();

    if (enable_tftp_)
    {
        bkp_data_write(BKP_DATA_1, 0xA5A5);
        Display::Get()->TextStatus("TFTP On ", console::Colours::kConsoleGreen);
    }
    else
    {
        bkp_data_write(BKP_DATA_1, 0x0);
        Display::Get()->TextStatus("TFTP Off", console::Colours::kConsoleGreen);
    }

    DEBUG_EXIT();
}

void RemoteConfig::PlatformHandleTftpGet()
{
    DEBUG_ENTRY();

    enable_tftp_ = (bkp_data_read(BKP_DATA_1) == 0xA5A5);

    DEBUG_PRINTF("enable_tftp_=%d", enable_tftp_);
    DEBUG_EXIT();
}
#endif