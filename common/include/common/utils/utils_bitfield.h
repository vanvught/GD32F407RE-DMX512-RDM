/**
 * @file utils_bitfield.h
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

#ifndef COMMON_UTILS_UTILS_BITFIELD_H_
#define COMMON_UTILS_UTILS_BITFIELD_H_

#include <cstdint>

namespace common {
template <class T>
void Set2BitField(uint32_t index, T value, uint16_t& packed) {
    packed &= static_cast<uint16_t>(~(0x3U << (index * 2U)));
    packed |= static_cast<uint16_t>((static_cast<uint32_t>(value) & 0x3U) << (index * 2U));
}

template <class T>
T Get2BitField(uint32_t index, uint16_t packed) {
    return static_cast<T>((packed >> (index * 2U)) & 0x3U);
}
} // namespace common

#endif // COMMON_UTILS_UTILS_BITFIELD_H_
