/**
 * @file time.cpp
 *
 */
/* Copyright (C) 2016-2026 by Arjan van Vught mailto:info@gd32-dmx.org
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

#include <cstdint>
#include <ctime>

namespace global {
int32_t g_utc_offset = 0;
} // namespace global

namespace {
constexpr int kDaysOfMonth[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

bool Isleapyear(int year) {
    if (year % 100 == 0) {
        return year % 400 == 0;
    }

    return year % 4 == 0;
}

int Getdaysofmonth(int month, int year) {
    if ((month == 1) && Isleapyear(year)) {
        return 29;
    }

    return kDaysOfMonth[month];
}

struct tm s_tm;
} // namespace

extern "C" {
struct tm* localtime(const time_t* _timer) { // NOLINT
    if (_timer == nullptr) {
        return nullptr;
    }

    auto time = *_timer + global::g_utc_offset;
    return gmtime(&time);
}

struct tm* gmtime(const time_t* _timer) { // NOLINT
    if (_timer == nullptr) {
        return nullptr;
    }

    if ((*_timer < 0) || (*_timer > UINT32_MAX)) {
        return nullptr;
    }

    auto time = static_cast<uint32_t>(*_timer);

    s_tm.tm_sec = static_cast<int>(time % 60U);
    time /= 60U;
    s_tm.tm_min = static_cast<int>(time % 60U);
    time /= 60U;
    s_tm.tm_hour = static_cast<int>(time % 24U);
    time /= 24U;

    s_tm.tm_wday = static_cast<int>((time + 4U) % 7U);

    int year = 1970;

    while (true) {
        const uint32_t days_of_year = Isleapyear(year) ? 366U : 365U;
        if (time < days_of_year) {
            break;
        }

        time -= days_of_year;
        year++;
    }

    s_tm.tm_year = year - 1900;
    s_tm.tm_yday = static_cast<int>(time);

    int month = 0;

    while (true) {
        const auto kDaysOfMonth = static_cast<uint32_t>(Getdaysofmonth(month, year));

        if (time < kDaysOfMonth) {
            break;
        }

        time -= kDaysOfMonth;
        month++;
    }

    s_tm.tm_mon = month;
    s_tm.tm_mday = static_cast<int>(time + 1U);

    return &s_tm;
}

// Uses malloc/free in Newlib
time_t mktime(struct tm* _timeptr) { // NOLINT
    if (_timeptr == nullptr) {
        return -1;
    }

    if ((_timeptr->tm_year < 70) || (_timeptr->tm_year > 139)) {
        return -1;
    }

    const int kTargetYear = 1900 + _timeptr->tm_year;
    uint32_t result = 0;

    for (int year = 1970; year < kTargetYear; year++) {
        result += Isleapyear(year) ? 366U : 365U;
    }

    if ((_timeptr->tm_mon < 0) || (_timeptr->tm_mon > 11)) {
        return -1;
    }

    for (int month = 0; month < _timeptr->tm_mon; month++) {
        result += static_cast<uint32_t>(Getdaysofmonth(month, kTargetYear));
    }

    if ((_timeptr->tm_mday < 1) || (_timeptr->tm_mday > Getdaysofmonth(_timeptr->tm_mon, kTargetYear))) {
        return -1;
    }

    result += static_cast<uint32_t>(_timeptr->tm_mday - 1);
    result *= 24U;

    if ((_timeptr->tm_hour < 0) || (_timeptr->tm_hour > 23)) {
        return -1;
    }

    result += static_cast<uint32_t>(_timeptr->tm_hour);
    result *= 60U;

    if ((_timeptr->tm_min < 0) || (_timeptr->tm_min > 59)) {
        return -1;
    }

    result += static_cast<uint32_t>(_timeptr->tm_min);
    result *= 60U;

    if ((_timeptr->tm_sec < 0) || (_timeptr->tm_sec > 59)) {
        return -1;
    }

    result += static_cast<uint32_t>(_timeptr->tm_sec);

    return static_cast<time_t>(result);
}
}
