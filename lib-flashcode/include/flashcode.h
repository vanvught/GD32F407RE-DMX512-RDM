/**
 * @file flashcode.h
 *
 */
/* Copyright (C) 2021-2026 by Arjan van Vught mailto:info@gd32-dmx.org
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

#ifndef FLASHCODE_H_
#define FLASHCODE_H_

#include <cstdint>
#include <span>

#include "firmware/debug/debug_debug.h"

#ifdef DEBUG_FLASHCODE
#define FLASHCODE_DEBUG_ENTRY() DEBUG_ENTRY()
#define FLASHCODE_DEBUG_EXIT() DEBUG_EXIT()
#define FLASHCODE_DEBUG_PRINTF(...) DEBUG_PRINTF(__VA_ARGS__)
#define FLASHCODE_DEBUG_PUTS(...) DEBUG_PUTS(__VA_ARGS__)
#else
#define FLASHCODE_DEBUG_ENTRY() \
    do {                        \
    } while (false)
#define FLASHCODE_DEBUG_EXIT() \
    do {                       \
    } while (false)
#define FLASHCODE_DEBUG_PRINTF(...) \
    do {                            \
    } while (false)
#define FLASHCODE_DEBUG_PUTS(...) \
    do {                          \
    } while (false)
#endif

#ifdef DEBUG_FMC
#define FMC_DEBUG_ENTRY() DEBUG_ENTRY()
#define FMC_DEBUG_EXIT() DEBUG_EXIT()
#define FMC_DEBUG_PRINTF(...) DEBUG_PRINTF(__VA_ARGS__)
#define FMC_DEBUG_PUTS(...) DEBUG_PUTS(__VA_ARGS__)
#else
#define FMC_DEBUG_ENTRY() \
    do {                  \
    } while (false)
#define FMC_DEBUG_EXIT() \
    do {                 \
    } while (false)
#define FMC_DEBUG_PRINTF(...) \
    do {                      \
    } while (false)
#define FMC_DEBUG_PUTS(...) \
    do {                    \
    } while (false)
#endif // DEBUG_COMMON_FMC

namespace flashcode {
enum class Result { kOk, kError };
} // namespace flashcode

class FlashCode {
   public:
    FlashCode();
    ~FlashCode();

    [[nodiscard]] bool IsDetected() const { return detected_; }

    [[nodiscard]] const char* GetName() const;
    [[nodiscard]] uint32_t GetSize() const;
    [[nodiscard]] uint32_t GetSectorSize() const;

    bool Read(uint32_t offset, std::span<uint8_t> buffer, flashcode::Result& result);
    bool Erase(uint32_t offset, uint32_t length, flashcode::Result& result);
    bool Write(uint32_t offset, std::span<const uint8_t> buffer, flashcode::Result& result);

    static FlashCode* Get() { return s_this; }

   private:
    bool detected_{false};
    inline static FlashCode* s_this;
};

#endif // FLASHCODE_H_
