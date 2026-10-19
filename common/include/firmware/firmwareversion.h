/**
 * @file firmwareversion.h
 */
/* Copyright (C) 2019-2026 by Arjan van Vught mailto:info@gd32-dmx.org
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

#ifndef FIRMWARE_FIRMWAREVERSION_H_
#define FIRMWARE_FIRMWAREVERSION_H_

#include <cstdio>
#include <string_view>
#include <cassert>

#include "board.h"

#if !defined(STR_HELPER)
#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)
#endif // STR_HELPER

namespace firmwareversion {
struct Info {
    std::string_view software_version;
    std::string_view build_date;
    std::string_view build_time;
};
} // namespace firmwareversion

class FirmwareVersion {
   public:
    constexpr FirmwareVersion(std::string_view version, std::string_view date, std::string_view time) : info_{.software_version = version, .build_date = date, .build_time = time} { s_this = this; }

    [[nodiscard]] constexpr const firmwareversion::Info& GetVersion() const noexcept { return info_; }

    void Print(const char* title = nullptr) const {
        printf("[V%.*s] %.*s Compiled on %.*s at %.*s\n", 
          static_cast<int>(info_.software_version.size()), 
          info_.software_version.data(), 
          static_cast<int>(board::BoardName().size()), 
          board::BoardName().data(),
          static_cast<int>(info_.build_date.size()), 
          info_.build_date.data(),
          static_cast<int>(info_.build_time.size()), 
          info_.build_time.data());

        if (title != nullptr) {
            printf("\x1b[32m%s\x1b[0m\n", title);
        }
    }

    static FirmwareVersion& Instance() {
        assert(s_this != nullptr);
        return *s_this;
    }

   private:
    firmwareversion::Info info_;
    static inline FirmwareVersion* s_this;
};

#endif // FIRMWARE_FIRMWAREVERSION_H_
