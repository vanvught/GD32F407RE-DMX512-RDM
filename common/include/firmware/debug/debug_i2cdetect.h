/**
 * @file debug_i2cdetect.h
 *
 */
/* Copyright (C) 2020-2026 by Arjan van Vught mailto:info@gd32-dmx.org
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

#ifndef FIRMWARE_DEBUG_DEBUG_I2CDETECT_H_
#define FIRMWARE_DEBUG_DEBUG_I2CDETECT_H_

#include <cstdint>
#include <cstdio>

#include "i2c.h"
#include "firmware/debug/debug_config.h"

namespace debug::i2c {
inline constexpr uint32_t kFirst = 0x03;
inline constexpr uint32_t kLast = 0x77;
inline constexpr uint32_t kTotalI2cAddresses = 128;
inline constexpr uint32_t kAddressesPerPage = 16;

void Detect() {
    if constexpr (!config::kI2cDetectEnabled) {
        return;
    }

    ::i2c::Begin();
    ::i2c::SetBaudrate(::i2c::kNormalSpeed);

    puts("\n     0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f");

    for (uint32_t i = 0; i < kTotalI2cAddresses; i = (i + kAddressesPerPage)) {
        printf("%02x: ", static_cast<unsigned>(i));
        for (uint32_t j = 0; j < kAddressesPerPage; j++) {
            uint32_t current_address = i + j;

            // Skip unwanted addresses
            if ((current_address < kFirst) || (current_address > kLast)) {
                printf("   ");
                continue;
            }

            if (::i2c::IsConnected(static_cast<uint8_t>(current_address))) {
                printf("%02x ", static_cast<unsigned>(current_address));
            } else {
                printf("-- ");
            }
        }

        puts("");
    }

    ::i2c::Begin();
}
} // namespace debug::i2c

#endif // FIRMWARE_DEBUG_DEBUG_I2CDETECT_H_
