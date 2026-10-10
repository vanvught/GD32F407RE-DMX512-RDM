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

#include <cstdint>
#include <cstdio>
#include <string_view>

#include "firmware/ansi_colour.h"

namespace common::print {
inline void Error(const char* func, const char* string) {
    printf("%s%s: %s%s\n", ansi::Colours::Fg::kRed, func, string, ansi::Colours::Fg::kDefault);
}

inline void Error(const char* func, std::string_view string) {
    printf("%s%s: %.*s%s\n", ansi::Colours::Fg::kRed, func, static_cast<int>(string.size()), string.data(), ansi::Colours::Fg::kDefault);
}

inline void Size(uint32_t size, std::string_view suffix = {}) {
    constexpr uint32_t kKiB{1024};
    constexpr uint32_t kMiB{1024 * kKiB};

    if (size >= kMiB) {
        printf("%u MiB", static_cast<unsigned>(size / kMiB));
    } else if (size >= kKiB) {
        printf("%u KiB", static_cast<unsigned>(size / kKiB));
    } else {
        printf("%u Bytes", static_cast<unsigned>(size));
    }

    printf("%.*s", static_cast<int>(suffix.size()), suffix.data());
}

inline void StringView(std::string_view s_v) {
    printf("%.*s\n", static_cast<int>(s_v.size()), s_v.data());
}
} // namespace common::print

#define ERROR(s)                             \
    do {                                     \
        common::print::Error(__func__, (s)); \
    } while (false)

#endif // COMMON_UTILS_UTILS_PRINT_H_
