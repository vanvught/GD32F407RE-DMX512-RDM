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

#include <cstdint>

namespace common {
constexpr uint32_t ConstStrLen(const char* str) {
    uint32_t len = 0;
    while (str[len] != '\0') {
        ++len;
    }
    return len;
}

inline int32_t Atoi(const char* buffer, uint32_t size) {
    const char* p = buffer;
    int32_t sign = 1;
    int32_t result = 0;

    if (size == 0) {
        return 0;
    }

    if (*p == '-') {
        sign = -1;
        p++;
        size--;
    } else if (*p == '+') {
        p++;
        size--;
    }

    for (; (size > 0) && (*p >= '0' && *p <= '9'); size--, p++) {
        result = (result * 10) + (*p - '0');
    }

    return sign * result;
}

inline float Atof(const char* buffer, uint32_t size) {
    const char* p = buffer;
    float sign = 1.0F;
    float result = 0.0F;

    if (size == 0) {
        return 0.0F;
    }

    if (*p == '-') {
        sign = -1.0F;
        ++p;
        --size;
    } else if (*p == '+') {
        ++p;
        --size;
    }

    while (size > 0 && *p >= '0' && *p <= '9') {
        result = (result * 10.0F) + static_cast<float>(*p - '0');
        ++p;
        --size;
    }

    if (size > 0 && *p == '.') {
        ++p;
        --size;

        float divisor = 10.0F;
        while (size > 0 && *p >= '0' && *p <= '9') {
            result += static_cast<float>(*p - '0') / divisor;
            divisor *= 10.0F;
            ++p;
            --size;
        }
    }

    return sign * result;
}
} // namespace common

#endif // COMMON_UTILS_UTILS_STRING_H_
