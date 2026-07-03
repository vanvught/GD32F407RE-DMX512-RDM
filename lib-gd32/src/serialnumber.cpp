/**
 * @file serialnumber.cpp
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

#include <cstdint>

#include "serialnumber.h"
#include "gd32_unique_id.h"

void SerialNumber(uint8_t sn[kSnSize]) {
    const auto kId0 = gd32::uid::Word0();

    sn[0] = static_cast<uint8_t>((kId0 >> 0) & 0xFF);
    sn[1] = static_cast<uint8_t>((kId0 >> 8) & 0xFF);
    sn[2] = static_cast<uint8_t>((kId0 >> 16) & 0xFF);
    sn[3] = static_cast<uint8_t>((kId0 >> 24) & 0xFF);
}
