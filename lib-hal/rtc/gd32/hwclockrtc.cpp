/**
 * @file  hwclockrtc.cpp
 *
 */
/* Copyright (C) 2021-2022 by Arjan van Vught mailto:info@gd32-dmx.org
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

#include <cassert>
#include <cstring>
#include <time.h>

#include "hardware.h"
#include "gd32.h"

#include "debug.h"

bool rtc_configuration(void) {
	rcu_osci_on (RCU_LXTAL);

	if (SUCCESS != rcu_osci_stab_wait(RCU_LXTAL)) {
		return false;
	}

	rcu_rtc_clock_config (RCU_RTCSRC_LXTAL);
	rcu_periph_clock_enable (RCU_RTC);
	rtc_register_sync_wait();
	rtc_lwoff_wait();
	rtc_prescaler_set(32767);
	rtc_lwoff_wait();

	return true;
}

using namespace rtc;

void HwClock::RtcProbe() {
	DEBUG_ENTRY

	if (bkp_data_read(BKP_DATA_0) != 0xA5A5) {
		DEBUG_PUTS("RTC not yet configured");

		if (!rtc_configuration()) {
			m_bIsConnected = false;
			DEBUG_PUTS("RTC did not start");
			DEBUG_EXIT
			return;
		}

		rtc_lwoff_wait();
		rtc_counter_set(time(nullptr));
		rtc_lwoff_wait();
		bkp_data_write(BKP_DATA_0, 0xA5A5);
	} else {
		DEBUG_PUTS("No need to configure RTC");
		rtc_register_sync_wait();
        rtc_lwoff_wait();
	}

	m_Type = rtc::Type::SOC_INTERNAL;
	m_bIsConnected = true;
	m_nLastHcToSysMillis = Hardware::Get()->Millis();

	DEBUG_EXIT
}

bool HwClock::RtcSet(const struct rtc_time *pRtcTime) {
	DEBUG_ENTRY
	assert(pRtcTime != nullptr);

	rtc_counter_set(mktime(const_cast<struct tm *>(reinterpret_cast<const struct tm *>(pRtcTime))));
//	rtc_lwoff_wait();

	DEBUG_EXIT
	return true;
}

bool HwClock::RtcGet(struct rtc_time *pRtcTime) {
	DEBUG_ENTRY
	assert(pRtcTime != nullptr);

	const auto nSeconds = static_cast<time_t>(rtc_counter_get());
	const auto *pTm = localtime(&nSeconds);
	memcpy(pRtcTime, pTm, sizeof(struct rtc_time));

	DEBUG_EXIT
	return true;
}
