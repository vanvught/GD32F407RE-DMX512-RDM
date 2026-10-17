/**
 * @file json_datetime.cpp
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

#include <cstdio>
#include <cstdint>
#include <ctime>
#include <sys/time.h>
#include <cassert>

#include "firmware/global.h"
#include "json/json_helpers.h"
#include "json/json_jsondoc.h"
#include "json/json_key.h"
#include "json/json_parser.h"
#include "common/utils/utils_string.h"
#include "firmware/utc.h"
#include "configstore.h"
#include "firmware/debug/debug_dump.h"
#include "firmware/debug/debug_debug.h"

namespace json {
namespace {
void SetDate(std::string_view date) {
    if (date.size() != 20 && date.size() != 25) {
        return;
    }

    struct tm tm{};

    tm.tm_year = common::Atoi(common::Slice(date, 0, 4)) - 1900;
    tm.tm_mon = common::Atoi(common::Slice(date, 5, 2)) - 1;
    tm.tm_mday = common::Atoi(common::Slice(date, 8, 2));
    tm.tm_hour = common::Atoi(common::Slice(date, 11, 2));
    tm.tm_min = common::Atoi(common::Slice(date, 14, 2));
    tm.tm_sec = common::Atoi(common::Slice(date, 17, 2));

    struct timeval time_val{.tv_sec = mktime(&tm), .tv_usec = 0};

    if (date.size() == 20) {
        assert(date[19] == 'Z');
    } else {
        const int32_t kSign = date[19] == '-' ? -1 : 1;

        const auto kHours = static_cast<int8_t>(common::Atoi(common::Slice(date, 20, 2)) * kSign);
        const auto kMinutes = static_cast<uint8_t>(common::Atoi(common::Slice(date, 23, 2)));

        DEBUG_PRINTF("[%d]%.2d:%.2d", static_cast<int>(kSign), static_cast<int>(kHours), static_cast<int>(kMinutes));

        int32_t utc_offset;

        if (utc::ValidateOffset(kHours, kMinutes, utc_offset)) {
            ConfigStore::Instance().GlobalUpdate(&common::store::Global::utc_offset, utc_offset);

            global::SetUtcOffsetIfValid(kHours, kMinutes);
        }

        time_val.tv_sec -= global::GetUtcOffset();
    }

    settimeofday(&time_val, nullptr);
}

constexpr auto kDate = json::MakeKey("date");

constexpr json::Key kActionKeys[] = {json::MakeKey(SetDate, kDate)};

} // namespace

uint32_t GetTimeofday(char* out_buffer, uint32_t out_buffer_size) {
    struct timeval time_val;
    if (gettimeofday(&time_val, nullptr) >= 0) {
        auto* local_time = localtime(&time_val.tv_sec);

        int32_t hours;
        uint32_t minutes;
        global::GetUtcOffset(hours, minutes);

        if ((hours == 0) && (minutes == 0)) {
            char date_time[20];
            common::DateTimeToString(*local_time, date_time);
            return json::helpers::Serialize(out_buffer, out_buffer_size, [&](JsonDoc& doc) {
                // For UTC, use a 21-byte buffer instead to append 'Z'.
                doc["date"] = date_time;
            });
        }

        char date_time[26];

        common::DateTimeIso8601(*local_time, hours, minutes, date_time);
        return json::helpers::Serialize(out_buffer, out_buffer_size, [&](JsonDoc& doc) { doc["date"] = date_time; });
    }

    return 0;
}

void SetTimeofday(const char* buffer, uint32_t buffer_size) {
    debug::Dump(buffer, buffer_size);
    ParseJsonWithTable(buffer, buffer_size, kActionKeys);
}
} // namespace json