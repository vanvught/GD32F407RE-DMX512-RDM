/**
 * @file  hwclockrtc.cpp
 */
/* Copyright (C) 2020-2026 by Arjan van Vught mailto:info@gd32-dmx.org
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

/**
 * MCP7941X: It has specific control bits like the Start Timer (ST) and battery enable (VBATEN).
 */

/**
 * DS3231: Known for its accuracy.
 */

/**
 * PCF8563: The time integrity is checked using the SECONDS_VL bit.
 *  Alarm: Minute, Hour, Day, Weekday
 */

#include <cassert>
#include <cstdint>
#include <ctime>

#include "hwclock.h"
#include "timing.h"
#include "i2c.h"
#include "common/utils/utils_bcd.h"

namespace rtc {
namespace reg {
static constexpr uint8_t kSeconds = 0x00;
static constexpr uint8_t kMinutes = 0x01;
static constexpr uint8_t kHours = 0x02;
static constexpr uint8_t kWday = 0x03;
static constexpr uint8_t kMday = 0x04;
static constexpr uint8_t kMonth = 0x05;
static constexpr uint8_t kYear = 0x06;
} // namespace reg
namespace mcp7941x {
namespace reg {
static constexpr uint8_t kControl = 0x07;
} // namespace reg
namespace bit {
static constexpr uint8_t kSt = 0x80;
static constexpr uint8_t kVbaten = 0x08;
static constexpr uint8_t kAlM0En = 0x10;
static constexpr uint8_t kAlmxIf = (1U << 3);
static constexpr uint8_t kAlmxC0 = (1U << 4);
static constexpr uint8_t kAlmxC1 = (1U << 5);
static constexpr uint8_t kAlmxC2 = (1U << 6);
#ifndef NDEBUG
static constexpr uint8_t kAlmxPol = (1U << 7);
#endif // NDEBUG
static constexpr uint8_t kMskAlmxMatch = (kAlmxC0 | kAlmxC1 | kAlmxC2);
} // namespace bit
} // namespace mcp7941x
namespace ds3231 {
namespace reg {
static constexpr uint8_t kAlarM1Seconds = 0x07;
static constexpr uint8_t kControl = 0x0e;
} // namespace reg
namespace bit {
static constexpr uint8_t kA1Ie = (1U << 0);
static constexpr uint8_t kA2Ie = (1U << 1);
static constexpr uint8_t kA1F = (1U << 0);
static constexpr uint8_t kA2F = (1U << 1);
} // namespace bit
} // namespace ds3231
namespace pcf8563 {
namespace reg {
static constexpr uint8_t kControlStatuS1 = 0x00;
static constexpr uint8_t kControlStatuS2 = 0x01;
static constexpr uint8_t kSeconds = 0x02;
// static constexpr uint8_t MINUTES			= 0x03;
// static constexpr uint8_t HOURS			= 0x04;
static constexpr uint8_t kMday = 0x05;
static constexpr uint8_t kWday = 0x06;
// static constexpr uint8_t MONTH			= 0x07;
static constexpr uint8_t kYear = 0x08;
static constexpr uint8_t kAlarm = 0x09;
} // namespace reg
namespace bit {
static constexpr uint8_t kSecondsVl = (1U << 7);
static constexpr uint8_t kStatus2Aie = (1U << 1); ///< alarm interrupt enabled
static constexpr uint8_t kStatus2Af = (1U << 3);  ///< read: alarm flag active
} // namespace bit
} // namespace pcf8563
namespace i2caddress {
static constexpr uint8_t kPcF8563 = 0x51;
static constexpr uint8_t kMcP7941X = 0x6F;
static constexpr uint8_t kDS3231 = 0x68;
} // namespace i2caddress
} // namespace rtc

namespace {
void AssertValidTime([[maybe_unused]] const struct tm& time) {
    assert(time.tm_sec >= 0 && time.tm_sec <= 59);
    assert(time.tm_min >= 0 && time.tm_min <= 59);
    assert(time.tm_hour >= 0 && time.tm_hour <= 23);
    assert(time.tm_wday >= 0 && time.tm_wday <= 6);
    assert(time.tm_mday >= 1 && time.tm_mday <= 31);
    assert(time.tm_mon >= 0 && time.tm_mon <= 11);
}
} // namespace

void HwClock::RtcProbe() {
    HWCLOCK_DEBUG_ENTRY();

    last_hc_to_sys_millis_ = timing::Millis();

    i2c::SetBaudrate(i2c::kNormalSpeed);

    uint8_t value;

#ifndef CONFIG_RTC_DISABLE_MCP7941X
    i2c::SetAddress(rtc::i2caddress::kMcP7941X);

    // The I2C bus is not stable at cold start? These dummy write/read helps.
    // This needs some more investigation for what is really happening here.
    i2c::ReadReg(rtc::reg::kYear, value);

    if (i2c::Write(nullptr, 0) == 0) {
        HWCLOCK_DEBUG_PUTS("MCP7941X");

        is_connected_ = true;
        type_ = rtc::Type::kMcP7941X;
        address_ = rtc::i2caddress::kMcP7941X;

        i2c::ReadReg(rtc::reg::kSeconds, value);

        if ((value & rtc::mcp7941x::bit::kSt) == 0) {
            HWCLOCK_DEBUG_PUTS("Start the on-board oscillator");

            struct tm rtc_time;

            rtc_time.tm_hour = 0;
            rtc_time.tm_min = 0;
            rtc_time.tm_sec = 0;
            rtc_time.tm_mday = _TIME_STAMP_DAY_;
            rtc_time.tm_mon = _TIME_STAMP_MONTH_ - 1;
            rtc_time.tm_year = _TIME_STAMP_YEAR_ - 1900;

            RtcSet(&rtc_time);
        }

        HWCLOCK_DEBUG_EXIT();
        return;
    }
#endif // CONFIG_RTC_DISABLE_MCP7941X

#ifndef CONFIG_RTC_DISABLE_DS3231
    i2c::SetAddress(rtc::i2caddress::kDS3231);

    // The I2C bus is not stable at cold start? These dummy write/read helps.
    // This needs some more investigation for what is really happening here.
    i2c::ReadReg(rtc::reg::kYear, value);

    if (i2c::Write(nullptr, 0) == 0) {
        HWCLOCK_DEBUG_PUTS("DS3231");

        is_connected_ = true;
        type_ = rtc::Type::kDS3231;
        address_ = rtc::i2caddress::kDS3231;

        struct tm tm;
        RtcGet(&tm);

        if ((tm.tm_hour > 24) || (tm.tm_year > _TIME_STAMP_YEAR_ - 1900)) {
            tm.tm_hour = 0;
            tm.tm_min = 0;
            tm.tm_sec = 0;
            tm.tm_mday = _TIME_STAMP_DAY_;
            tm.tm_mon = _TIME_STAMP_MONTH_ - 1;
            tm.tm_year = _TIME_STAMP_YEAR_ - 1900;

            RtcSet(&tm);
        }

        HWCLOCK_DEBUG_EXIT();
        return;
    }
#endif // CONFIG_RTC_DISABLE_DS3231

#ifndef CONFIG_RTC_DISABLE_PCF8563
    i2c::SetAddress(rtc::i2caddress::kPcF8563);

    // The I2C bus is not stable at cold start? These dummy write/read helps.
    // This needs some more investigation for what is really happening here.
    i2c::ReadReg(rtc::pcf8563::reg::kYear, value);

    if (i2c::Write(nullptr, 0) == 0) {
        HWCLOCK_DEBUG_PUTS("PCF8563");

        is_connected_ = true;
        type_ = rtc::Type::kPcF8563;
        address_ = rtc::i2caddress::kPcF8563;

        i2c::WriteReg(rtc::pcf8563::reg::kControlStatuS1, static_cast<uint8_t>(0));
        i2c::WriteReg(rtc::pcf8563::reg::kControlStatuS2, static_cast<uint8_t>(0));

        i2c::ReadReg(rtc::pcf8563::reg::kSeconds, value);

        // Register seconds has the VL bit
        // 0 - clock integrity is guaranteed
        // 1 - integrity of the clock information is not guaranteed
        if ((value & rtc::pcf8563::bit::kSecondsVl) == rtc::pcf8563::bit::kSecondsVl) {
            HWCLOCK_DEBUG_PUTS("Integrity of the clock information is not guaranteed");

            struct tm rtc_time;

            rtc_time.tm_hour = 0;
            rtc_time.tm_min = 0;
            rtc_time.tm_sec = 0;
            rtc_time.tm_mday = _TIME_STAMP_DAY_;
            rtc_time.tm_mon = _TIME_STAMP_MONTH_ - 1;
            rtc_time.tm_year = _TIME_STAMP_YEAR_ - 1900;

            RtcSet(&rtc_time);
        }

        i2c::ReadReg(rtc::pcf8563::reg::kSeconds, value);

        if ((value & rtc::pcf8563::bit::kSecondsVl) == rtc::pcf8563::bit::kSecondsVl) {
            HWCLOCK_DEBUG_PUTS("Clock is not running -> disconnected");
            is_connected_ = false;
        }

        HWCLOCK_DEBUG_EXIT();
        return;
    }
#endif // CONFIG_RTC_DISABLE_PCF8563

    HWCLOCK_DEBUG_EXIT();
}

bool HwClock::RtcSet(const struct tm* time) {
    HWCLOCK_DEBUG_ENTRY();
    assert(time != nullptr);

    if (!is_connected_) {
        HWCLOCK_DEBUG_EXIT();
        return false;
    }

    HWCLOCK_DEBUG_PRINTF("secs=%d, mins=%d, hours=%d, mday=%d, mon=%d, year=%d, wday=%d", time->tm_sec, time->tm_min, time->tm_hour, time->tm_mday, time->tm_mon, time->tm_year, time->tm_wday);

    AssertValidTime(*time);

    char data[8];
    auto* registers = &data[1];

    registers[rtc::reg::kSeconds] = common::bcd::FromDecimal(static_cast<uint8_t>(time->tm_sec));
    registers[rtc::reg::kMinutes] = common::bcd::FromDecimal(static_cast<uint8_t>(time->tm_min));
    registers[rtc::reg::kHours] = common::bcd::FromDecimal(static_cast<uint8_t>(time->tm_hour));

#ifndef CONFIG_RTC_DISABLE_PCF8563
    if (type_ == rtc::Type::kPcF8563) {
        registers[rtc::pcf8563::reg::kWday - rtc::pcf8563::reg::kSeconds] = common::bcd::FromDecimal(static_cast<uint8_t>(time->tm_wday));
        registers[rtc::pcf8563::reg::kMday - rtc::pcf8563::reg::kSeconds] = common::bcd::FromDecimal(static_cast<uint8_t>(time->tm_mday));
    } else
#endif // CONFIG_RTC_DISABLE_PCF8563
    {
        registers[rtc::reg::kWday] = common::bcd::FromDecimal(static_cast<uint8_t>(time->tm_wday));
        registers[rtc::reg::kMday] = common::bcd::FromDecimal(static_cast<uint8_t>(time->tm_mday));
    }

    registers[rtc::reg::kMonth] = common::bcd::FromDecimal(static_cast<uint8_t>(time->tm_mon + 1));
    registers[rtc::reg::kYear] = common::bcd::FromDecimal(static_cast<uint8_t>(time->tm_year - 100));

    if (type_ == rtc::Type::kMcP7941X) {
        registers[rtc::reg::kSeconds] |= rtc::mcp7941x::bit::kSt;
        registers[rtc::reg::kWday] |= rtc::mcp7941x::bit::kVbaten;
    }

    if (type_ == rtc::Type::kPcF8563) {
        data[0] = rtc::pcf8563::reg::kSeconds;
    } else {
        data[0] = rtc::reg::kSeconds;
    }

    i2c::SetAddress(address_);
    i2c::SetBaudrate(i2c::kFullSpeed);
    i2c::Write(data, sizeof(data) / sizeof(data[0]));

    HWCLOCK_DEBUG_EXIT();
    return true;
}

bool HwClock::RtcGet(struct tm* time) {
    HWCLOCK_DEBUG_ENTRY();
    assert(time != nullptr);

    if (!is_connected_) {
        HWCLOCK_DEBUG_EXIT();
        return false;
    }

    char registers[7];

    if (type_ == rtc::Type::kPcF8563) {
        registers[0] = rtc::pcf8563::reg::kSeconds;
    } else {
        registers[0] = rtc::reg::kSeconds;
    }

    i2c::SetAddress(address_);
    i2c::SetBaudrate(i2c::kFullSpeed);
    i2c::Write(registers, 1);
    i2c::Read(registers, sizeof(registers) / sizeof(registers[0]));

    time->tm_sec = common::bcd::ToDecimal(registers[rtc::reg::kSeconds] & 0x7f);
    time->tm_min = common::bcd::ToDecimal(registers[rtc::reg::kMinutes] & 0x7f);
    time->tm_hour = common::bcd::ToDecimal(registers[rtc::reg::kHours] & 0x3f);

#ifndef CONFIG_RTC_DISABLE_PCF8563
    if (type_ == rtc::Type::kPcF8563) {
        time->tm_wday = common::bcd::ToDecimal(registers[rtc::pcf8563::reg::kWday - rtc::pcf8563::reg::kSeconds] & 0x07);
        time->tm_mday = common::bcd::ToDecimal(registers[rtc::pcf8563::reg::kMday - rtc::pcf8563::reg::kSeconds] & 0x3f);
    } else
#endif // CONFIG_RTC_DISABLE_PCF8563
    {
        time->tm_wday = common::bcd::ToDecimal(registers[rtc::reg::kWday] & 0x07);
        time->tm_mday = common::bcd::ToDecimal(registers[rtc::reg::kMday] & 0x3f);
    }

    time->tm_mon = common::bcd::ToDecimal(registers[rtc::reg::kMonth] & 0x1f) - 1;
    time->tm_year = common::bcd::ToDecimal(registers[rtc::reg::kYear]) + 100;

    HWCLOCK_DEBUG_PRINTF("secs=%d, mins=%d, hours=%d, mday=%d, mon=%d, year=%d, wday=%d", time->tm_sec, time->tm_min, time->tm_hour, time->tm_mday, time->tm_mon, time->tm_year, time->tm_wday);

    HWCLOCK_DEBUG_EXIT();
    return true;
}

bool HwClock::RtcSetAlarm(const struct tm* time) {
    HWCLOCK_DEBUG_ENTRY();
    assert(time != nullptr);

    HWCLOCK_DEBUG_PRINTF("secs=%d, mins=%d, hours=%d, mday=%d, mon=%d, year=%d, wday=%d", time->tm_sec, time->tm_min, time->tm_hour, time->tm_mday, time->tm_mon, time->tm_year, time->tm_wday);

    AssertValidTime(*time);

    switch (type_) {
#ifndef CONFIG_RTC_DISABLE_MCP7941X
        case rtc::Type::kMcP7941X: {
            const auto kWday = static_cast<char>(MCP794xxAlarmWeekday(const_cast<struct tm*>(time)));

            // Read control and alarm 0 registers.
            char registers[11];
            auto data = &registers[1];
            data[0] = rtc::mcp7941x::reg::kControl;

            i2c::Write(data, 1);
            i2c::Read(data, 10);

            // Set alarm 0, using 24-hour and day-of-month modes.
            data[3] = common::bcd::FromDecimal(static_cast<uint8_t>(time->tm_sec));
            data[4] = common::bcd::FromDecimal(static_cast<uint8_t>(time->tm_min));
            data[5] = common::bcd::FromDecimal(static_cast<uint8_t>(time->tm_hour));
            data[6] = kWday;
            data[7] = common::bcd::FromDecimal(static_cast<uint8_t>(time->tm_mday));
            data[8] = common::bcd::FromDecimal(static_cast<uint8_t>(time->tm_mon + 1));
            // Clear the alarm 0 interrupt flag.
            data[6] &= static_cast<char>(~rtc::mcp7941x::bit::kAlmxIf);
            // Set alarm match: second, minute, hour, day, date, month.
            data[6] |= rtc::mcp7941x::bit::kMskAlmxMatch;
            // Disable interrupt. We will not enable until completely programmed.
            data[0] &= static_cast<char>(~rtc::mcp7941x::bit::kAlM0En);

            registers[0] = rtc::mcp7941x::reg::kControl;
            i2c::Write(registers, sizeof(registers) / sizeof(registers[0]));

            if (alarm_enabled_) {
                registers[0] = rtc::mcp7941x::reg::kControl;
                registers[1] |= rtc::mcp7941x::bit::kAlM0En;
                i2c::Write(registers, 2);
            }

            HWCLOCK_DEBUG_EXIT();
            return true;
        } break;
#endif // CONFIG_RTC_DISABLE_MCP7941X
#ifndef CONFIG_RTC_DISABLE_DS3231
        case rtc::Type::kDS3231: {
            char registers[10];
            auto data = &registers[1];
            data[0] = rtc::ds3231::reg::kAlarM1Seconds;

            i2c::SetAddress(address_);
            i2c::SetBaudrate(i2c::kFullSpeed);
            i2c::Write(data, 1);
            i2c::Read(data, 9);

            const auto kControl = data[7];
            const auto kStatus = data[8];

            // set ALARM1, using 24 hour and day-of-month modes
            data[0] = common::bcd::FromDecimal(static_cast<uint8_t>(time->tm_sec));
            data[1] = common::bcd::FromDecimal(static_cast<uint8_t>(time->tm_min));
            data[2] = common::bcd::FromDecimal(static_cast<uint8_t>(time->tm_hour));
            data[3] = common::bcd::FromDecimal(static_cast<uint8_t>(time->tm_mday));
            // set ALARM2 to non-garbage
            data[4] = 0;
            data[5] = 0;
            data[6] = 0;
            // disable alarms
            data[7] = kControl & static_cast<char>(~(rtc::ds3231::bit::kA1Ie | rtc::ds3231::bit::kA2Ie));
            data[8] = kStatus & static_cast<char>(~(rtc::ds3231::bit::kA1F | rtc::ds3231::bit::kA2F));

            registers[0] = rtc::ds3231::reg::kAlarM1Seconds;
            i2c::Write(registers, sizeof(registers) / sizeof(registers[0]));

            if (alarm_enabled_) {
                HWCLOCK_DEBUG_PUTS("Alarm is enabled");
                registers[0] = rtc::ds3231::reg::kControl;
                registers[1] |= rtc::ds3231::bit::kA1Ie;
                i2c::Write(registers, 2);
            }

            HWCLOCK_DEBUG_EXIT();
            return true;
        } break;
#endif // CONFIG_RTC_DISABLE_DS3231
#ifndef CONFIG_RTC_DISABLE_PCF8563
        case rtc::Type::kPcF8563: {
            char data[5];

            data[0] = rtc::pcf8563::reg::kAlarm;
            data[1] = common::bcd::FromDecimal(static_cast<uint8_t>(time->tm_min));
            data[2] = common::bcd::FromDecimal(static_cast<uint8_t>(time->tm_hour));
            data[3] = common::bcd::FromDecimal(static_cast<uint8_t>(time->tm_mday));
            data[4] = time->tm_wday & 0x07;

            i2c::SetAddress(address_);
            i2c::SetBaudrate(i2c::kFullSpeed);
            i2c::Write(data, sizeof(data) / sizeof(data[0]));

            PCF8563SetAlarmMode();

            HWCLOCK_DEBUG_EXIT();
            return true;
        } break;
#endif // CONFIG_RTC_DISABLE_PCF8563
        default:
            break;
    }

    HWCLOCK_DEBUG_EXIT();
    return false;
}

bool HwClock::RtcGetAlarm(struct tm* time) {
    HWCLOCK_DEBUG_ENTRY();
    assert(time != nullptr);

    if (!RtcGet(time)) {
        HWCLOCK_DEBUG_EXIT();
        return false;
    }

    switch (type_) {
#ifndef CONFIG_RTC_DISABLE_MCP7941X
        case rtc::Type::kMcP7941X: {
            char registers[10];

            registers[0] = rtc::mcp7941x::reg::kControl;

            i2c::SetAddress(address_);
            i2c::SetBaudrate(i2c::kFullSpeed);
            i2c::Write(registers, 1);
            i2c::Read(registers, sizeof(registers) / sizeof(registers[0]));

            time->tm_sec = common::bcd::ToDecimal(registers[3] & 0x7f);
            time->tm_min = common::bcd::ToDecimal(registers[4] & 0x7f);
            time->tm_hour = common::bcd::ToDecimal(registers[5] & 0x3f);
            time->tm_wday = common::bcd::ToDecimal(registers[6] & 0x7) - 1;
            time->tm_mday = common::bcd::ToDecimal(registers[7] & 0x3f);
            time->tm_mon = common::bcd::ToDecimal(registers[8] & 0x1f) - 1;

            alarm_enabled_ = registers[0] & rtc::mcp7941x::bit::kAlM0En;

            HWCLOCK_DEBUG_PRINTF("sec=%d min=%d hour=%d wday=%d mday=%d mon=%d enabled=%d irq=%d match=%u", time->tm_sec, time->tm_min, time->tm_hour, time->tm_wday, time->tm_mday, time->tm_mon, alarm_enabled_,
                                 (registers[6] & rtc::mcp7941x::bit::kAlmxIf), (registers[6] & rtc::mcp7941x::bit::kMskAlmxMatch) >> 4);

            HWCLOCK_DEBUG_EXIT();
            return true;
        } break;
#endif // CONFIG_RTC_DISABLE_MCP7941X
#ifndef CONFIG_RTC_DISABLE_DS3231
        case rtc::Type::kDS3231: {
            char registers[10];

            registers[0] = rtc::ds3231::reg::kAlarM1Seconds;

            i2c::SetAddress(address_);
            i2c::SetBaudrate(i2c::kFullSpeed);
            i2c::Write(registers, 1);
            i2c::Read(registers, sizeof(registers) / sizeof(registers[0]));

            time->tm_sec = common::bcd::ToDecimal(registers[0] & 0x7f);
            time->tm_min = common::bcd::ToDecimal(registers[1] & 0x7f);
            time->tm_hour = common::bcd::ToDecimal(registers[2] & 0x3f);
            time->tm_mday = common::bcd::ToDecimal(registers[3] & 0x3f);

            alarm_enabled_ = (registers[7] & rtc::ds3231::bit::kA1Ie);
            alarm_pending_ = (registers[8] & rtc::ds3231::bit::kA1F);

            HWCLOCK_DEBUG_PRINTF("tm is sec=%d, min=%d, hour=%d, mday=%d, enabled=%d, pending=%d", time->tm_sec, time->tm_min, time->tm_hour, time->tm_mday, alarm_enabled_, alarm_pending_);

            HWCLOCK_DEBUG_EXIT();
            return true;
        }

        break;
#endif // CONFIG_RTC_DISABLE_DS3231
#ifndef CONFIG_RTC_DISABLE_PCF8563
        case rtc::Type::kPcF8563: {
            char registers[4];

            registers[0] = rtc::pcf8563::reg::kAlarm;

            i2c::SetAddress(address_);
            i2c::SetBaudrate(i2c::kFullSpeed);
            i2c::Write(registers, 1);
            i2c::Read(registers, sizeof(registers) / sizeof(registers[0]));

            HWCLOCK_DEBUG_PRINTF("raw data is min=%02x, hr=%02x, mday=%02x, wday=%02x", registers[0], registers[1], registers[2], registers[3]);

            time->tm_sec = 0;
            time->tm_min = common::bcd::ToDecimal(registers[0] & 0x7F);
            time->tm_hour = common::bcd::ToDecimal(registers[1] & 0x3F);
            time->tm_mday = common::bcd::ToDecimal(registers[2] & 0x3F);
            time->tm_wday = common::bcd::ToDecimal(registers[3] & 0x7);

            PCF8563GetAlarmMode();

            HWCLOCK_DEBUG_PRINTF("tm is mins=%d, hours=%d, mday=%d, wday=%d, enabled=%d, pending=%d", time->tm_min, time->tm_hour, time->tm_mday, time->tm_wday, alarm_enabled_, alarm_pending_);

            HWCLOCK_DEBUG_EXIT();
            return true;
        } break;
#endif // CONFIG_RTC_DISABLE_PCF8563
        default:
            break;
    }

    HWCLOCK_DEBUG_EXIT();
    return false;
}

int HwClock::MCP794xxAlarmWeekday(struct tm* time) {
    HWCLOCK_DEBUG_ENTRY();
    assert(time != nullptr);
    assert(type_ == rtc::Type::kMcP7941X);

    struct tm tm_now;
    RtcGet(&tm_now);

    const auto kDaysNow = mktime(&tm_now) / (24 * 60 * 60);
    const auto kDaysAlarm = mktime(time) / (24 * 60 * 60);
    const auto kReturn = static_cast<int>(((tm_now.tm_wday + kDaysAlarm - kDaysNow) % 7) + 1);

    HWCLOCK_DEBUG_EXIT();
    return kReturn;
}

void HwClock::PCF8563GetAlarmMode() {
    HWCLOCK_DEBUG_ENTRY();
    assert(type_ == rtc::Type::kPcF8563);

    uint8_t value;
    i2c::ReadReg(rtc::pcf8563::reg::kControlStatuS2, value);

    alarm_enabled_ = (value & rtc::pcf8563::bit::kStatus2Aie) == rtc::pcf8563::bit::kStatus2Aie;
    alarm_pending_ = (value & rtc::pcf8563::bit::kStatus2Af) == rtc::pcf8563::bit::kStatus2Af;

    HWCLOCK_DEBUG_EXIT();
}

void HwClock::PCF8563SetAlarmMode() const {
    HWCLOCK_DEBUG_ENTRY();
    assert(type_ == rtc::Type::kPcF8563);

    char data[2];

    data[1] = rtc::pcf8563::reg::kControlStatuS2;

    i2c::Write(&data[1], 1);
    i2c::Read(&data[1], 1);

    if (alarm_enabled_) {
        HWCLOCK_DEBUG_PUTS("Alarm is enabled");
        data[1] |= rtc::pcf8563::bit::kStatus2Aie;
    } else {
        data[1] &= static_cast<char>(~rtc::pcf8563::bit::kStatus2Aie);
    }

    data[0] = rtc::pcf8563::reg::kControlStatuS2;
    data[1] &= static_cast<char>(~rtc::pcf8563::bit::kStatus2Af);

    i2c::Write(data, 2);

    HWCLOCK_DEBUG_EXIT();
}
