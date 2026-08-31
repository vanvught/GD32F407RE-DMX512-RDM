/**
 * @file json_parsehelper.h
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

#ifndef JSON_JSON_PARSEHELPER_H_
#define JSON_JSON_PARSEHELPER_H_

#include <cstdint>
#include <type_traits>

#include "common/utils/utils_string.h"

namespace json {
template <typename T>
T ParseValue(const char* val, uint32_t len) {
    int32_t value = common::Atoi(val, len);
    if constexpr (std::is_unsigned_v<T>) {
        if (value < 0) {
            return 0; // or handle error
        }
    }
    return static_cast<T>(value);
}

template <typename T>
bool ParseInRange(const char* val, uint32_t len, T min, T max, T* out) {
    const auto kValue = ParseValue<T>(val, len);

    if ((kValue < min) || (kValue > max)) {
        return false;
    }

    *out = kValue;
    return true;
}

template <typename ParseT, typename StoreT>
bool ParseInRange(const char* val, uint32_t len, ParseT min, ParseT max, StoreT* out) {
    const auto kValue = ParseValue<ParseT>(val, len);

    if ((kValue < min) || (kValue > max)) {
        return false;
    }

    *out = static_cast<StoreT>(kValue);
    return true;
}
} // namespace json

#endif // JSON_JSON_PARSEHELPER_H_
