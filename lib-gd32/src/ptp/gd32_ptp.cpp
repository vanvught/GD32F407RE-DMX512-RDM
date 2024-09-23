/**
 * @file gd32_ptp.cpp
 *
 */
/* Copyright (C) 2024 by Arjan van Vught mailto:info@gd32-dmx.org
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
#include <cstring>
#include <time.h>
#include <sys/time.h>

#include "gd32_ptp.h"
#include "gd32.h"

#include "debug.h"

extern "C" void console_error(const char *);

#if defined (GD32H7XX)
# define enet_interrupt_disable(x)					enet_interrupt_disable(ENETx, x)
# define enet_ptp_feature_enable(x)					enet_ptp_feature_enable(ENETx, x)
# define enet_ptp_subsecond_increment_config(x) 	enet_ptp_subsecond_increment_config(ENETx, x)
# define enet_ptp_timestamp_function_config(x)		enet_ptp_timestamp_function_config(ENETx, x)
# define enet_ptp_timestamp_addend_config(x)		enet_ptp_timestamp_addend_config(ENETx, x)
# define enet_ptp_timestamp_update_config(x,y,z)	enet_ptp_timestamp_update_config(ENETx, x, y, z)
# define enet_ptp_system_time_get(x)				enet_ptp_system_time_get(ENETx, x)
static FlagStatus enet_ptpflagstatus_get(const uint32_t flag) {
	FlagStatus bitstatus = RESET;

	if (0 != (ENET_PTP_TSCTL(ENETx) & flag)) {
		bitstatus = SET;
	}

	return bitstatus;
}
#else
static FlagStatus enet_ptpflagstatus_get(const uint32_t flag) {
	FlagStatus bitstatus = RESET;

	if (0 != (ENET_PTP_TSCTL & flag)) {
		bitstatus = SET;
	}

	return bitstatus;
}
#endif

static void ptp_start(const uint32_t init_sec, const uint32_t init_subsec, [[maybe_unused]] const uint32_t carry_cfg, const uint32_t accuracy_cfg) {
	DEBUG_ENTRY

	enet_interrupt_disable(ENET_MAC_INT_TMSTIM);

#if defined(GD32F4XX) || defined (GD32H7XX)
	enet_ptp_feature_enable(ENET_ALL_RX_TIMESTAMP | ENET_RXTX_TIMESTAMP);
#else
	enet_ptp_feature_enable(ENET_RXTX_TIMESTAMP);
#endif
	enet_ptp_subsecond_increment_config(accuracy_cfg);

	enet_ptp_timestamp_addend_config(carry_cfg);

	enet_ptp_timestamp_function_config(ENET_PTP_ADDEND_UPDATE);
	while(enet_ptpflagstatus_get(ENET_PTP_ADDEND_UPDATE) == SET);

	enet_ptp_timestamp_function_config(ENET_PTP_FINEMODE);

	enet_ptp_timestamp_update_config(ENET_PTP_ADD_TO_TIME, init_sec, init_subsec);
	enet_ptp_timestamp_function_config(ENET_PTP_SYSTIME_INIT);
	while(enet_ptpflagstatus_get(ENET_PTP_SYSTIME_INIT) == SET);

	DEBUG_EXIT
}

void gd32_ptp_start() {
	DEBUG_ENTRY
	DEBUG_PRINTF("PTP_TICK=%u", gd32::ptp::PTP_TICK);
	DEBUG_PRINTF("ADJ_FREQ_BASE_INCREMENT=%u", gd32::ptp::ADJ_FREQ_BASE_INCREMENT);
	DEBUG_PRINTF("ADJ_FREQ_BASE_ADDEND=x%X", gd32::ptp::ADJ_FREQ_BASE_ADDEND);

	struct tm tmbuf;
	memset(&tmbuf, 0, sizeof(struct tm));
	tmbuf.tm_mday = _TIME_STAMP_DAY_;			// The day of the month, in the range 1 to 31.
	tmbuf.tm_mon = _TIME_STAMP_MONTH_ - 1;		// The number of months since January, in the range 0 to 11.
	tmbuf.tm_year = _TIME_STAMP_YEAR_ - 1900;	// The number of years since 1900.

	ptp_start(mktime(&tmbuf), 0, gd32::ptp::ADJ_FREQ_BASE_ADDEND, gd32::ptp::ADJ_FREQ_BASE_INCREMENT);

#ifndef NDEBUG
	struct timeval tv;
	gettimeofday(&tv, nullptr);
	auto *tm = localtime(&tv.tv_sec);

	DEBUG_PRINTF("%.2d-%.2d-%.4d %.2d:%.2d:%.2d.%.6d", tm->tm_mday, tm->tm_mon + 1, tm->tm_year + 1900, tm->tm_hour, tm->tm_min, tm->tm_sec, static_cast<int>(tv.tv_usec));
#endif
	DEBUG_EXIT
}

void gd32_ptp_get_time(gd32::ptp::ptptime *ptp_time) {
	enet_ptp_systime_struct systime;

	enet_ptp_system_time_get(&systime);

	ptp_time->tv_sec = systime.second;
#if !defined (GD32F4XX)
	ptp_time->tv_nsec = systime.nanosecond;
#else
	ptp_time->tv_nsec = gd32::ptp_subsecond_2_nanosecond(systime.subsecond);
#endif
}

void gd32_ptp_set_time(const gd32::ptp::ptptime *ptp_time) {
	const auto nSign = ENET_PTP_ADD_TO_TIME;
	const auto nSecond = ptp_time->tv_sec;
	const auto nNanoSecond = ptp_time->tv_nsec;
	const auto nSubSecond = gd32::ptp_nanosecond_2_subsecond(nNanoSecond);

	enet_ptp_timestamp_update_config(nSign, nSecond, nSubSecond);
	enet_ptp_timestamp_function_config(ENET_PTP_SYSTIME_INIT);
	while(enet_ptpflagstatus_get(ENET_PTP_SYSTIME_INIT) == SET);
}

void gd32_ptp_update_time(const gd32::ptp::time_t *pTime) {
	  uint32_t nSign;
	  uint32_t nSecond;
	  uint32_t nNanoSecond;

	if (pTime->tv_sec < 0 || (pTime->tv_sec == 0 && pTime->tv_nsec < 0)) {
		nSign = ENET_PTP_SUBSTRACT_FROM_TIME;
		nSecond = -pTime->tv_sec;
		nNanoSecond = -pTime->tv_nsec;
	} else {
		nSign = ENET_PTP_ADD_TO_TIME;
		nSecond = pTime->tv_sec;
		nNanoSecond = pTime->tv_nsec;
	}

	const auto nSubSecond = gd32::ptp_nanosecond_2_subsecond(nNanoSecond);
#if defined (GD32H7XX)
	const auto nAddend = ENET_PTP_TSADDEND(ENETx);
#else
    const auto nAddend = ENET_PTP_TSADDEND;
#endif

	enet_ptp_timestamp_update_config(nSign, nSecond, nSubSecond);
    enet_ptp_timestamp_function_config(ENET_PTP_SYSTIME_UPDATE);
    while(enet_ptpflagstatus_get(ENET_PTP_SYSTIME_UPDATE) == SET);

    enet_ptp_timestamp_addend_config(nAddend);
    enet_ptp_timestamp_function_config(ENET_PTP_ADDEND_UPDATE);
}

bool gd32_adj_frequency(const int32_t adjust_ppb) {
	const uint32_t addend = gd32::ptp::ADJ_FREQ_BASE_ADDEND + static_cast<int32_t>((((static_cast<int64_t>(gd32::ptp::ADJ_FREQ_BASE_ADDEND)) * adjust_ppb) / 1000000000ULL));

	enet_ptp_timestamp_addend_config(addend);

	const auto reval = enet_ptp_timestamp_function_config(ENET_PTP_ADDEND_UPDATE);

	if (reval == ERROR) {
		console_error("enet_ptp_timestamp_addend_config\n");
	}

	return reval != ERROR;
}
