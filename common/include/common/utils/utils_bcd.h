/**
 * @file utils_bcd.h
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

#ifndef COMMON_UTILS_UTILS_BCD_H_
#define COMMON_UTILS_UTILS_BCD_H_

#include <cstdint>

namespace common::bcd {

[[nodiscard]] constexpr uint8_t ToDecimal(uint8_t value) {
    return static_cast<uint8_t>((value & 0x0FU) + ((value >> 4U) * 10U));
}

[[nodiscard]] constexpr uint8_t FromDecimal(uint8_t value) {
    return static_cast<uint8_t>(((value / 10U) << 4U) | (value % 10U));
}

[[nodiscard]] constexpr bool IsValid(uint8_t value) {
    return (value & 0x0FU) <= 9U && ((value >> 4U) & 0x0FU) <= 9U;
}
} // namespace common::bcd

#endif // COMMON_UTILS_UTILS_BCD_H_
