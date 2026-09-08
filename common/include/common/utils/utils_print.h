/**
 * @file utils_print.h
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

#ifndef COMMON_UTILS_UTILS_PRINT_H_
#define COMMON_UTILS_UTILS_PRINT_H_

#include <cstdio>

#include "firmware/ansi_colour.h"

namespace common::print {
inline void Error(const char* func, const char* string) {
    printf("%s%s: %s%s\n", ansi::Colours::Fg::kRed, func, string, ansi::Colours::Fg::kDefault);
}
} // namespace common::print

#define ERROR(s)                             \
    do {                                     \
        common::print::Error(__func__, (s)); \
    } while (false)

#endif // COMMON_UTILS_UTILS_PRINT_H_
