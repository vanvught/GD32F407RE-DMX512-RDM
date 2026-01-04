/**
 * @file  hwclockrtc.cpp
 *
 */
/* Copyright (C) 2021-2025 by Arjan van Vught mailto:info@gd32-dmx.org
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


#if defined(GD32F4XX) || defined(GD32H7XX)
#else
#include <cstring>
#endif
#include <cassert>
#include <time.h>

#include "hwclock.h"
#include "gd32.h"
#include "gd32_millis.h"
#include "firmware/debug/debug_debug.h"

#define BCD2DEC(val) (((val) & 0x0f) + ((val) >> 4) * 10)
#define DEC2BCD(val) static_cast<char>((((val) / 10) << 4) + (val) % 10)

#if defined(GD32F4XX) || defined(GD32H7XX)
#define RTC_CLOCK_SOURCE_LXTAL
static rtc_parameter_struct rtc_initpara;
#endif

#if defined(GD32F4XX) || defined(GD32H7XX)
static bool RtcConfiguration()
{
#if defined(RTC_CLOCK_SOURCE_IRC32K)
    rcu_osci_on(RCU_IRC32K);
    if (SUCCESS != rcu_osci_stab_wait(RCU_IRC32K))
    {
        return false;
    }
    rcu_rtc_clock_config(RCU_RTCSRC_IRC32K);
#elif defined(RTC_CLOCK_SOURCE_LXTAL)
    rcu_osci_on(RCU_LXTAL);

    if (SUCCESS != rcu_osci_stab_wait(RCU_LXTAL))
    {
        return false;
    }

    rcu_rtc_clock_config(RCU_RTCSRC_LXTAL);
#else
#error RTC clock source should be defined.
#endif
    rcu_periph_clock_enable(RCU_RTC);

    if (SUCCESS != rtc_register_sync_wait())
    {
        return false;
    }

    rtc_initpara.date = DEC2BCD(_TIME_STAMP_DAY_);
    rtc_initpara.month = DEC2BCD(_TIME_STAMP_MONTH_ - 1);
    rtc_initpara.year = DEC2BCD(_TIME_STAMP_YEAR_ - 1900);

    if (SUCCESS != rtc_init(&rtc_initpara))
    {
        DEBUG_PUTS("RTC time configuration failed!");
        return false;
    }

    DEBUG_PUTS("RTC time configuration success!");
    return true;
}
#else
static bool RtcConfiguration()
{
    rcu_osci_on(RCU_LXTAL);

    if (SUCCESS != rcu_osci_stab_wait(RCU_LXTAL))
    {
        return false;
    }

    rcu_rtc_clock_config(RCU_RTCSRC_LXTAL);
    rcu_periph_clock_enable(RCU_RTC);
    rtc_register_sync_wait();
    rtc_lwoff_wait();
    rtc_prescaler_set(32767);
    rtc_lwoff_wait();

    return true;
}
#endif

void HwClock::RtcProbe()
{
    DEBUG_ENTRY();

#if defined(GD32F4XX) || defined(GD32H7XX)
#if defined(RTC_CLOCK_SOURCE_IRC32K)
    rtc_initpara.factor_syn = 0x13F;
    rtc_initpara.factor_asyn = = 0x63;
#elif defined(RTC_CLOCK_SOURCE_LXTAL)
    rtc_initpara.factor_syn = 0xFF;
    rtc_initpara.factor_asyn = 0x7F;
#else
#error RTC clock source should be defined.
#endif
    rtc_initpara.display_format = RTC_24HOUR;
#endif

    if (bkp_data_read(BKP_DATA_0) != 0xA5A5)
    {
        DEBUG_PUTS("RTC not yet configured");

        if (!RtcConfiguration())
        {
            is_connected_ = false;
            DEBUG_PUTS("RTC did not start");
            DEBUG_EXIT();
            return;
        }

        bkp_data_write(BKP_DATA_0, 0xA5A5);

        struct tm rtc_time;

        rtc_time.tm_hour = 0;
        rtc_time.tm_min = 0;
        rtc_time.tm_sec = 0;
        rtc_time.tm_mday = _TIME_STAMP_DAY_;
        rtc_time.tm_mon = _TIME_STAMP_MONTH_ - 1;
        rtc_time.tm_year = _TIME_STAMP_YEAR_ - 1900;

        RtcSet(&rtc_time);
    }
    else
    {
        DEBUG_PUTS("No need to configure RTC");
        rtc_register_sync_wait();
#if defined(GD32F4XX) || defined(GD32H7XX)
#else
        rtc_lwoff_wait();
#endif
    }

    type_ = rtc::Type::kSocInternal;
    is_connected_ = true;
    last_hc_to_sys_millis_ = millis();

    DEBUG_EXIT();
}

bool HwClock::RtcSet(const struct tm* tm_time)
{
    assert(tm_time != nullptr);

    DEBUG_PRINTF("sec=%d, min=%d, hour=%d, mday=%d, mon=%d, year=%d, wday=%d", tm_time->tm_sec, tm_time->tm_min, tm_time->tm_hour, tm_time->tm_mday, tm_time->tm_mon, tm_time->tm_year, tm_time->tm_wday);

#if defined(GD32F4XX) || defined(GD32H7XX)
    rtc_initpara.year = DEC2BCD(tm_time->tm_year);
    rtc_initpara.month = DEC2BCD(tm_time->tm_mon);
    rtc_initpara.date = DEC2BCD(tm_time->tm_mday);
    rtc_initpara.day_of_week = DEC2BCD(tm_time->tm_wday);
    rtc_initpara.hour = DEC2BCD(tm_time->tm_hour);
    rtc_initpara.minute = DEC2BCD(tm_time->tm_min);
    rtc_initpara.second = DEC2BCD(tm_time->tm_sec);

    return (SUCCESS == rtc_init(&rtc_initpara));
#else
    rtc_counter_set(mktime(const_cast<struct tm*>(tm_time)));
#endif
    return true;
}

bool HwClock::RtcGet(struct tm* tm_time)
{
    assert(tm_time != nullptr);

#if defined(GD32F4XX) || defined(GD32H7XX)
    const auto kTr = reinterpret_cast<uint32_t>(RTC_TIME);
    const auto kDr = reinterpret_cast<uint32_t>(RTC_DATE);

    tm_time->tm_year = BCD2DEC(GET_DATE_YR(kDr));
    tm_time->tm_mon = BCD2DEC(GET_DATE_MON(kDr));
    tm_time->tm_mday = BCD2DEC(GET_DATE_DAY(kDr));
    tm_time->tm_wday = BCD2DEC(GET_DATE_DOW(kDr));
    tm_time->tm_hour = BCD2DEC(GET_TIME_HR(kTr));
    tm_time->tm_min = BCD2DEC(GET_TIME_MN(kTr));
    tm_time->tm_sec = BCD2DEC(GET_TIME_SC(kTr));
#else
    const auto kSeconds = static_cast<time_t>(rtc_counter_get());
    const auto* pTm = gmtime(&kSeconds);
    memcpy(tm_time, pTm, sizeof(struct tm));
#endif

    DEBUG_PRINTF("sec=%d, min=%d, hour=%d, mday=%d, mon=%d, year=%d, wday=%d", tm_time->tm_sec, tm_time->tm_min, tm_time->tm_hour, tm_time->tm_mday, tm_time->tm_mon, tm_time->tm_year, tm_time->tm_wday);

    return true;
}

bool HwClock::RtcSetAlarm(const struct tm* tm_time)
{
    DEBUG_ENTRY();
    assert(tm_time != nullptr);

    DEBUG_PRINTF("secs=%d, mins=%d, hours=%d, mday=%d, mon=%d, year=%d, wday=%d, enabled=%d", tm_time->tm_sec, tm_time->tm_min, tm_time->tm_hour, tm_time->tm_mday, tm_time->tm_mon, tm_time->tm_year, tm_time->tm_wday, alarm_enabled_);

#if defined(GD32F4XX) || defined(GD32H7XX)
    rtc_alarm_disable(RTC_ALARM0);
    rtc_alarm_struct rtc_alarm;

    rtc_alarm.alarm_mask = RTC_ALARM_ALL_MASK;
    rtc_alarm.weekday_or_date = RTC_ALARM_DATE_SELECTED;
    rtc_alarm.am_pm = 0;
    rtc_alarm.alarm_day = DEC2BCD(tm_time->tm_mday);
    rtc_alarm.alarm_hour = DEC2BCD(tm_time->tm_hour);
    rtc_alarm.alarm_minute = DEC2BCD(tm_time->tm_min);
    rtc_alarm.alarm_second = DEC2BCD(tm_time->tm_sec);

    rtc_alarm_config(RTC_ALARM0, &rtc_alarm);

    if (alarm_enabled_)
    {
        rtc_interrupt_enable(RTC_INT_ALARM0);
        rtc_alarm_enable(RTC_ALARM0);
    }
    else
    {
        rtc_alarm_disable(RTC_ALARM0);
        rtc_interrupt_disable(RTC_INT_ALARM0);
    }
#else
    rtc_alarm_config(mktime(const_cast<struct tm*>(tm_time)));
#endif

    DEBUG_EXIT();
    return true;
}

bool HwClock::RtcGetAlarm(struct tm* tm_time)
{
    DEBUG_ENTRY();
    assert(tm_time != nullptr);

#if defined(GD32F4XX) || defined(GD32H7XX)
    if (!RtcGet(tm_time))
    {
        DEBUG_EXIT();
        return false;
    }

    rtc_alarm_struct rtc_alarm;
    rtc_alarm_get(RTC_ALARM0, &rtc_alarm);

    tm_time->tm_sec = BCD2DEC(rtc_alarm.alarm_second);
    tm_time->tm_min = BCD2DEC(rtc_alarm.alarm_minute);
    tm_time->tm_hour = BCD2DEC(rtc_alarm.alarm_hour);
    tm_time->tm_mday = BCD2DEC(rtc_alarm.alarm_day);
#else
    const auto kSeconds = static_cast<time_t>((RTC_ALRMH << 16U) | RTC_ALRML);
    const auto* lt = localtime(&kSeconds);
    memcpy(tm_time, lt, sizeof(struct tm));
#endif

    DEBUG_PRINTF("secs=%d, mins=%d, hours=%d, mday=%d, mon=%d, year=%d, wday=%d, enabled=%d", tm_time->tm_sec, tm_time->tm_min, tm_time->tm_hour, tm_time->tm_mday, tm_time->tm_mon, tm_time->tm_year, tm_time->tm_wday, alarm_enabled_);

    DEBUG_EXIT();
    return true;
}
