/**
 * @file debug_printbits.h
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

#ifndef FIRMWARE_DEBUG_DEBUG_PRINTBITS_H_
#define FIRMWARE_DEBUG_DEBUG_PRINTBITS_H_

#include <concepts>
#include <cstdio>
#include <climits>

#include "firmware/debug/debug_config.h"

namespace debug {
template <typename T>
    requires std::unsigned_integral<T>
inline void PrintBits(T value, const char* string = nullptr) {
    if constexpr (!config::kDumpEnabled) {
        return;
    }

    constexpr int kTotalBits = sizeof(T) * CHAR_BIT;
    constexpr int kMaxBitIndex = kTotalBits - 1;
    constexpr int kHexDigits = kTotalBits / 4; // 4 bits per hex character

    static_assert(sizeof(T) <= sizeof(unsigned));

    if (string != nullptr) {
        printf("%s :", string);
    }
    printf("0x%.*x ", kHexDigits, static_cast<unsigned>(value));

    for (int bit_number = kMaxBitIndex; bit_number >= 0; --bit_number) {
        const auto kMask = static_cast<T>(1) << bit_number;

        if ((value & kMask) != 0U) {
            printf("%d ", bit_number);
        }
    }

    putchar('\n');
}
} // namespace debug

#endif // FIRMWARE_DEBUG_DEBUG_PRINTBITS_H_
