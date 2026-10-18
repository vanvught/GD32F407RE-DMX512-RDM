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
#include <cstring>
#include <ctime>
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

inline char* AppendDecimal(char* out, uint8_t value) noexcept {
    if (value >= 100U) {
        *out++ = static_cast<char>('0' + value / 100U);
        value = static_cast<uint8_t>(value % 100U);
        *out++ = static_cast<char>('0' + value / 10U);
    } else if (value >= 10U) {
        *out++ = static_cast<char>('0' + value / 10U);
    }

    *out++ = static_cast<char>('0' + value % 10U);
    return out;
}

inline char* Append2(char* p, uint32_t value) noexcept {
    *p++ = static_cast<char>('0' + value / 10U);
    *p++ = static_cast<char>('0' + value % 10U);
    return p;
}

inline char* Append4(char* p, uint32_t value) noexcept {
    p = Append2(p, value / 100U);
    return Append2(p, value % 100U);
}

inline char* AppendUnsigned(char* p, uint32_t value) noexcept {
    char digits[10];
    uint32_t count = 0;

    do {
        digits[count++] = static_cast<char>('0' + value % 10U);
        value /= 10U;
    } while (value != 0U);

    while (count != 0U) {
        *p++ = digits[--count];
    }

    return p;
}

inline char* AppendFixed(char* p, float value, uint32_t decimals) noexcept {
    assert(decimals <= 2U);
    assert(value >= 0.0F);

    const uint32_t kScale = decimals == 2U ? 100U : 10U;
    const auto kScaled = static_cast<uint32_t>((value * static_cast<float>(kScale)) + 0.5F);

    p = AppendUnsigned(p, kScaled / kScale);
    *p++ = '.';

    if (decimals == 2U) {
        return Append2(p, kScaled % kScale);
    }

    *p++ = static_cast<char>('0' + kScaled % 10U);
    return p;
}

inline uint32_t DateTimeToString(const tm& value, char (&buffer)[20]) noexcept {
    auto* p = buffer;

    p = Append4(p, static_cast<uint32_t>(1900 + value.tm_year));
    *p++ = '-';
    p = Append2(p, static_cast<uint32_t>(1 + value.tm_mon));
    *p++ = '-';
    p = Append2(p, static_cast<uint32_t>(value.tm_mday));
    *p++ = 'T';
    p = Append2(p, static_cast<uint32_t>(value.tm_hour));
    *p++ = ':';
    p = Append2(p, static_cast<uint32_t>(value.tm_min));
    *p++ = ':';
    p = Append2(p, static_cast<uint32_t>(value.tm_sec));

    *p = '\0';
    return static_cast<uint32_t>(p - buffer);
}

inline uint32_t DateTimeIso8601(const tm& value, int32_t offset_hours, uint32_t offset_minutes, char (&buffer)[26]) noexcept {
    auto* p = buffer;

    p = Append4(p, static_cast<uint32_t>(1900 + value.tm_year));
    *p++ = '-';
    p = Append2(p, static_cast<uint32_t>(1 + value.tm_mon));
    *p++ = '-';
    p = Append2(p, static_cast<uint32_t>(value.tm_mday));
    *p++ = 'T';
    p = Append2(p, static_cast<uint32_t>(value.tm_hour));
    *p++ = ':';
    p = Append2(p, static_cast<uint32_t>(value.tm_min));
    *p++ = ':';
    p = Append2(p, static_cast<uint32_t>(value.tm_sec));

    if (offset_hours == 0 && offset_minutes == 0) {
        *p++ = 'Z';
    } else {
        *p++ = offset_hours < 0 ? '-' : '+';

        const auto kHours = static_cast<uint32_t>(offset_hours < 0 ? -offset_hours : offset_hours);

        p = Append2(p, kHours);
        *p++ = ':';
        p = Append2(p, offset_minutes);
    }

    *p = '\0';
    return static_cast<uint32_t>(p - buffer);
}

inline uint32_t IpToString(uint32_t ip, char (&buffer)[16]) noexcept {
    auto* p = buffer;

    for (uint32_t shift = 0; shift < 32U; shift += 8U) {
        if (shift != 0U) {
            *p++ = '.';
        }

        p = AppendDecimal(p, static_cast<uint8_t>(ip >> shift));
    }

    *p = '\0';
    return static_cast<uint32_t>(p - buffer);
}

class Writer {
   public:
    Writer(char* buffer, uint32_t capacity) noexcept : buffer_(buffer), capacity_(capacity) {}

    void Append(std::string_view value) noexcept {
        if (!ok_) {
            return;
        }

        if (size_ > capacity_ || value.size() > capacity_ - size_) {
            ok_ = false;
            return;
        }

        if (!value.empty()) {
            std::memcpy(buffer_ + size_, value.data(), value.size());
            size_ += static_cast<uint32_t>(value.size());
        }
    }

    void AppendUnsigned(uint32_t value) noexcept {
        char digits[10];
        auto* end = common::AppendUnsigned(digits, value);
        Append(std::string_view{digits, static_cast<size_t>(end - digits)});
    }

    void AppendIp(uint32_t ip) noexcept {
        char text[16];
        const auto kLength = common::IpToString(ip, text);
        Append(std::string_view{text, kLength});
    }

    [[nodiscard]] bool Ok() const noexcept { return ok_; }

    [[nodiscard]] uint32_t Size() const noexcept { return ok_ ? size_ : 0U; }

   private:
    char* buffer_;
    uint32_t capacity_;
    uint32_t size_{0};
    bool ok_{true};
};
} // namespace common

#endif // COMMON_UTILS_UTILS_STRING_H_
