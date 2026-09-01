/**
 * @file utils_math.h
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

#ifndef COMMON_UTILS_UTILS_MATH_H_
#define COMMON_UTILS_UTILS_MATH_H_

// <algorithm> is not part of freestanding C++23

namespace common {
template <typename T>
constexpr T Min(T a, T b) {
    return b < a ? b : a;
}

template <typename T>
constexpr T Max(T a, T b) {
    return a < b ? b : a;
}

template <class T, class Compare>
constexpr const T& Clamp(const T& value, const T& low, const T& high, Compare comp) {
    return comp(value, low) ? low : comp(high, value) ? high : value;
}

template <class T>
constexpr const T& Clamp(const T& value, const T& low, const T& high) {
    return clamp(value, low, high, [](const T& a, const T& b) { return a < b; });
}
} // namespace common

#endif // UTILS_UTILS_MATH_H_
