/**
 * @file hal_serialnumber.cpp
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

#include <cstdint>

#include "hal_serialnumber.h"

namespace hal
{
void SerialNumber(uint8_t sn[kSnSize])
{
#if defined(GD32H7XX)
    const auto kMacaddressHigh = *reinterpret_cast<volatile uint32_t*>(0x1FF0F7E8);
#elif defined(GD32F4XX)
    const auto kMacaddressHigh = *reinterpret_cast<volatile uint32_t*>(0x1FFF7A10);
#else
    const auto kMacaddressHigh = *reinterpret_cast<volatile uint32_t*>(0x1FFFF7E8);
#endif

    sn[0] = static_cast<uint8_t>((kMacaddressHigh >> 0) & 0xFF);
    sn[1] = static_cast<uint8_t>((kMacaddressHigh >> 8) & 0xFF);
    sn[2] = static_cast<uint8_t>((kMacaddressHigh >> 16) & 0xFF);
    sn[3] = static_cast<uint8_t>((kMacaddressHigh >> 24) & 0xFF);
}
} // namespace hal