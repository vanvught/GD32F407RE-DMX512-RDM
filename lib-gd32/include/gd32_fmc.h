/**
 * @file gd32_fmc.h
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

#ifndef GD32_FMC_H_
#define GD32_FMC_H_

#include <cstdint>
#include <span>

#ifdef GD32F4XX
#define FMC_SIZE (*reinterpret_cast<uint16_t*>(0x1FFF7A22U))
#endif

#ifdef GD32H7XX
#define FMC_SIZE (((REG32(0x1FF0F7E0) >> 16) & 0xFFFF) * 1024U)
#endif

namespace gd32::fmc {
enum class Result { kOk = 0, kError = 1 };

// Blocking API's
bool Read(uint32_t offset, std::span<uint8_t> buffer);
bool Erase(uint32_t offset, uint32_t length);
bool Write(uint32_t offset, std::span<const uint8_t> buffer);
// State-machine API's
bool Erase(uint32_t offset, uint32_t length, Result& result);
bool Write(uint32_t offset, std::span<const uint8_t> buffer, Result& result);
} // namespace gd32::fmc

#endif // GD32_FMC_H_
