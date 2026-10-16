/**
 * @file utils_string.h
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

#ifndef COMMON_UTILS_UTILS_STRING_H_
#define COMMON_UTILS_UTILS_STRING_H_

#include <cassert>
#include <cstdint>
#include <string_view>

namespace common {
inline constexpr char kWarning[] = "Warning";
inline constexpr char kError[] = "Error";
inline constexpr char kSuccess[] = "Success";
inline constexpr char kUnknown[] = "Unknown";
inline constexpr char kUndefined[] = "Undefined";

constexpr const char* IsSuccess(bool is_success) {
    return is_success ? kSuccess : kError;
}

inline int32_t Atoi(std::string_view value) {
    int32_t sign = 1;
    int32_t result = 0;

    if (value.empty()) {
        return 0;
    }

    if (value.front() == '-') {
        sign = -1;
        value.remove_prefix(1);
    } else if (value.front() == '+') {
        value.remove_prefix(1);
    }

    for (const auto kCh : value) {
        if ((kCh < '0') || (kCh > '9')) {
            break;
        }

        result = (result * 10) + (kCh - '0');
    }

    return sign * result;
}

inline float Atof(std::string_view buffer) {
    float sign = 1.0F;
    float result = 0.0F;

    if (buffer.empty()) {
        return 0.0F;
    }

    if (buffer.front() == '-') {
        sign = -1.0F;
        buffer.remove_prefix(1);
    } else if (buffer.front() == '+') {
        buffer.remove_prefix(1);
    }

    while (!buffer.empty() && buffer.front() >= '0' && buffer.front() <= '9') {
        result = (result * 10.0F) + static_cast<float>(buffer.front() - '0');
        buffer.remove_prefix(1);
    }

    if (!buffer.empty() && buffer.front() == '.') {
        buffer.remove_prefix(1);

        float divisor = 10.0F;
        while (!buffer.empty() && buffer.front() >= '0' && buffer.front() <= '9') {
            result += static_cast<float>(buffer.front() - '0') / divisor;
            divisor *= 10.0F;
            buffer.remove_prefix(1);
        }
    }

    return sign * result;
}

[[nodiscard]] constexpr std::string_view Slice(std::string_view value, size_t offset, size_t count) noexcept {
    assert(offset <= value.size());
    assert(count <= value.size() - offset);

    value.remove_prefix(offset);
    return std::string_view{value.data(), count}; // NOLINT(bugprone-suspicious-stringview-data-usage)
}

template <typename T, std::size_t N>
void CopyString(T (&destination)[N], std::string_view value) {
    static_assert(N > 0);
    static_assert(std::is_same_v<T, char> || std::is_same_v<T, uint8_t>);

    const auto kLength = std::min(value.size(), N - 1);

    if (kLength != 0) {
        memcpy(destination, value.data(), kLength);
    }

    memset(destination + kLength, 0, N - kLength);
}
} // namespace common

#endif // COMMON_UTILS_UTILS_STRING_H_
