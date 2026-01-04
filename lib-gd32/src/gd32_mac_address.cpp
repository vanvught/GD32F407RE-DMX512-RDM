/**
 * @file gd32_mac_address.cpp
 *
 */
/* Copyright (C) 2021-2025 by Arjan van Vught mailto:info@gd32-dmx.org
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

#if defined(DEBUG_MACADDRESS)
#undef NDEBUG
#endif

#include <cstdint>

#include "firmware/debug/debug_debug.h"

void mac_address_get(uint8_t paddr[])
{
#if defined(GD32H7XX)
    const auto kMacaddressHigh = *reinterpret_cast<volatile uint32_t*>(0x1FF0F7E8);
    const auto kMacAddressLow = *reinterpret_cast<volatile uint32_t*>(0x1FF0F7EC);
#elif defined(GD32F4XX)
    const auto kMacaddressHigh = *reinterpret_cast<volatile uint32_t*>(0x1FFF7A10);
    const auto kMacAddressLow = *reinterpret_cast<volatile uint32_t*>(0x1FFF7A14);
#else
    const auto kMacaddressHigh = *reinterpret_cast<volatile uint32_t*>(0x1FFFF7E8);
    const auto kMacAddressLow = *reinterpret_cast<volatile uint32_t*>(0x1FFFF7EC);
#endif

    paddr[0] = 2;
    paddr[1] = static_cast<uint8_t>((kMacAddressLow >> 0) & 0xFF);
    paddr[2] = static_cast<uint8_t>((kMacaddressHigh >> 24) & 0xFF);
    paddr[3] = static_cast<uint8_t>((kMacaddressHigh >> 16) & 0xFF);
    paddr[4] = static_cast<uint8_t>((kMacaddressHigh >> 8) & 0xFF);
    paddr[5] = static_cast<uint8_t>((kMacaddressHigh >> 0) & 0xFF);

    DEBUG_PRINTF("%02x:%02x:%02x:%02x:%02x:%02x", paddr[0], paddr[1], paddr[2], paddr[3], paddr[4], paddr[5]);
}
