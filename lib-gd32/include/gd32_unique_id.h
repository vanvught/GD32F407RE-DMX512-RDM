/**
 * @file gd32_unique_id.h
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

#ifndef GD32_UNIQUE_ID_H_
#define GD32_UNIQUE_ID_H_

#include <cstdint>

namespace gd32::uid {
#if defined(GD32H7XX)
static constexpr uintptr_t kBase = 0x1FF0F7E8;
#elif defined(GD32F4XX)
static constexpr uintptr_t kBase = 0x1FFF7A10;
#else
static constexpr uintptr_t kBase = 0x1FFFF7E8;
#endif

static inline uint32_t Word(uint32_t index) {
    return *reinterpret_cast<volatile const uint32_t*>(kBase + (index * sizeof(uint32_t)));
}

static inline uint32_t Word0() {
    return Word(0);
}
static inline uint32_t Word1() {
    return Word(1);
}
static inline uint32_t Word2() {
    return Word(2);
}
} // namespace gd32::uid

#endif // GD32_UNIQUE_ID_H_
