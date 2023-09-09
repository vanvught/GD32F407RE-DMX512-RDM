/**
 * @file hardware.cpp
 *
 */
/* Copyright (C) 2021-2023 by Arjan van Vught mailto:info@gd32-dmx.org
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of thnDmxDataDirecte Software, and to permit persons to whom the Software is
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
#include <cassert>

#include "hardware.h"

#include "gd32.h"
#include "gd32_i2c.h"
#include "gd32_adc.h"
#include "gd32_board.h"

#if defined (DEBUG_I2C)
# include "../debug/i2c/i2cdetect.h"
#endif

#if defined (ENABLE_USB_HOST)
void usb_init();
#endif

#include "debug.h"

extern "C" {
void console_init(void);
void __libc_init_array(void);
void systick_config(void);
}

void udelay_init();
void gd32_adc_init();

Hardware *Hardware::s_pThis = nullptr;

Hardware::Hardware() {
	DEBUG_ENTRY
	assert(s_pThis == nullptr);
	s_pThis = this;

	console_init();
#if !defined (ENABLE_TFTP_SERVER)
	__libc_init_array();
#endif
	systick_config();
    udelay_init();
	gd32_adc_init();
	gd32_i2c_begin();

	rcu_periph_clock_enable(RCU_TIMER5);
	timer_deinit(TIMER5);
	timer_parameter_struct timer_initpara;
	timer_initpara.prescaler = TIMER_PSC_1MHZ;
	timer_initpara.period = static_cast<uint32_t>(~0);
	timer_init(TIMER5, &timer_initpara);
	timer_enable(TIMER5);

#if !defined (GD32F4XX)
#else
	rcu_timer_clock_prescaler_config(RCU_TIMER_PSC_MUL4);
#endif

#ifndef NDEBUG
	const auto nSYS = rcu_clock_freq_get(CK_SYS);
	const auto nAHB = rcu_clock_freq_get(CK_AHB);
	const auto nAPB1 = rcu_clock_freq_get(CK_APB1);
	const auto nAPB2 = rcu_clock_freq_get(CK_APB2);
	printf("CK_SYS=%u\nCK_AHB=%u\nCK_APB1=%u\nCK_APB2=%u\n", nSYS, nAHB, nAPB1, nAPB2);
	assert(nSYS == MCU_CLOCK_FREQ);
	assert(nAPB1 == APB1_CLOCK_FREQ);
	assert(nAPB2 == APB2_CLOCK_FREQ);
#endif

#if !defined (GD32F4XX)
	rcu_periph_clock_enable (RCU_BKPI);
	rcu_periph_clock_enable (RCU_PMU);
	pmu_backup_write_enable();
#else
	rcu_periph_clock_enable(RCU_PMU);
	pmu_backup_ldo_config(PMU_BLDOON_ON);
	rcu_periph_clock_enable(RCU_BKPSRAM);
	pmu_backup_write_enable();
#endif
	bkp_data_write(BKP_DATA_1, 0x0);

#if !defined (ENABLE_TFTP_SERVER)
# if defined (GD32F207RG) || defined (GD32F4XX)
	// clear section .dmx
	extern unsigned char _sdmx;
	extern unsigned char _edmx;
	DEBUG_PRINTF("clearing .dmx at %p, size %u", &_sdmx, &_edmx - &_sdmx);
	memset(&_sdmx, 0, &_edmx - &_sdmx);
#  if defined (GD32F450VI)
	// clear section .lightset
	extern unsigned char _slightset;
	extern unsigned char _elightset;
	DEBUG_PRINTF("clearing .lightset at %p, size %u", &_slightset, &_elightset - &_slightset);
	memset(&_slightset, 0, &_elightset - &_slightset);
#  endif
	// clear section .network
	extern unsigned char _snetwork;
	extern unsigned char _enetwork;
	DEBUG_PRINTF("clearing .network at %p, size %u", &_snetwork, &_enetwork - &_snetwork);
	memset(&_snetwork, 0, &_enetwork - &_snetwork);
#  if !defined (GD32F450VE)
	// clear section .pixel
	extern unsigned char _spixel;
	extern unsigned char _epixel;
	DEBUG_PRINTF("clearing .pixel at %p, size %u", &_spixel, &_epixel - &_spixel);
	memset(&_spixel, 0, &_epixel - &_spixel);
#  endif
# endif
#else
# if defined (GD32F207RG) || defined (GD32F4XX)
	// clear section .network
	extern unsigned char _snetwork;
	extern unsigned char _enetwork;
	DEBUG_PRINTF("clearing .network at %p, size %u", &_snetwork, &_enetwork - &_snetwork);
	memset(&_snetwork, 0, &_enetwork - &_snetwork);
# endif
#endif

	struct tm tmbuf;

	tmbuf.tm_hour = 0;
	tmbuf.tm_min = 0;
	tmbuf.tm_sec = 0;
	tmbuf.tm_mday = _TIME_STAMP_DAY_;			// The day of the month, in the range 1 to 31.
	tmbuf.tm_mon = _TIME_STAMP_MONTH_ - 1;		// The number of months since January, in the range 0 to 11.
	tmbuf.tm_year = _TIME_STAMP_YEAR_ - 1900;	// The number of years since 1900.
	tmbuf.tm_isdst = 0; 						// 0 (DST not in effect, just take RTC time)

	const auto seconds = mktime(&tmbuf);
	const struct timeval tv = { seconds, 0 };

	settimeofday(&tv, nullptr);

#if defined (DEBUG_I2C)
	I2cDetect i2cdetect;
#endif

#if !defined(DISABLE_RTC)
	m_HwClock.RtcProbe();
	m_HwClock.Print();
	m_HwClock.HcToSys();
#endif

#if !defined(USE_LEDBLINK_BITBANGING595)
	rcu_periph_clock_enable(LED_BLINK_GPIO_CLK);
# if !defined (GD32F4XX)
	gpio_init(LED_BLINK_GPIO_PORT, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, LED_BLINK_PIN);
# else
	gpio_mode_set(LED_BLINK_GPIO_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, LED_BLINK_PIN);
	gpio_output_options_set(LED_BLINK_GPIO_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, LED_BLINK_PIN);
# endif
	GPIO_BC(LED_BLINK_GPIO_PORT) = LED_BLINK_PIN;
#endif

#if defined (LEDPANEL_595_CS_GPIOx)
	rcu_periph_clock_enable(LEDPANEL_595_CS_RCU_GPIOx);
# if !defined (GD32F4XX)
	gpio_init(LEDPANEL_595_CS_GPIOx, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, LEDPANEL_595_CS_GPIO_PINx);
# else
	gpio_mode_set(LEDPANEL_595_CS_GPIOx, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, LEDPANEL_595_CS_GPIO_PINx);
	gpio_output_options_set(LEDPANEL_595_CS_GPIOx, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, LEDPANEL_595_CS_GPIO_PINx);
# endif
	GPIO_BOP(LEDPANEL_595_CS_GPIOx) = LEDPANEL_595_CS_GPIO_PINx;
#endif

#if defined ENABLE_USB_HOST
	usb_init();
#endif

	DEBUG_EXIT
}

typedef union pcast32 {
	uuid_t uuid;
	uint32_t u32[4];
} _pcast32;

void Hardware::GetUuid(uuid_t out) {
	_pcast32 cast;

#if !defined (GD32F4XX)
	cast.u32[0] = *(volatile uint32_t*) (0x1FFFF7E8);
	cast.u32[1] = *(volatile uint32_t*) (0x1FFFF7EC);
	cast.u32[2] = *(volatile uint32_t*) (0x1FFFF7F0);
#else
	cast.u32[0] = *(volatile uint32_t*) (0x1FFF7A10);
	cast.u32[1] = *(volatile uint32_t*) (0x1FFF7A14);
	cast.u32[2] = *(volatile uint32_t*) (0x1FFF7A18);
#endif
	cast.u32[3] = cast.u32[0] + cast.u32[1] + cast.u32[2];

	memcpy(out, cast.uuid, sizeof(uuid_t));
}

bool Hardware::SetTime(__attribute__((unused)) const struct tm *pTime) {
	DEBUG_ENTRY
#if !defined(DISABLE_RTC)
	rtc_time rtc_time;

	rtc_time.tm_sec = pTime->tm_sec;
	rtc_time.tm_min = pTime->tm_min;
	rtc_time.tm_hour = pTime->tm_hour;
	rtc_time.tm_mday = pTime->tm_mday;
	rtc_time.tm_mon = pTime->tm_mon;
	rtc_time.tm_year = pTime->tm_year;

	m_HwClock.Set(&rtc_time);

	DEBUG_EXIT
	return true;
#else
	DEBUG_EXIT
	return false;
#endif
}

void Hardware::GetTime(struct tm *pTime) {
	auto ltime = time(nullptr);
	const auto *local_time = localtime(&ltime);

	pTime->tm_year = local_time->tm_year;
	pTime->tm_mon = local_time->tm_mon;
	pTime->tm_mday = local_time->tm_mday;
	pTime->tm_hour = local_time->tm_hour;
	pTime->tm_min = local_time->tm_min;
	pTime->tm_sec = local_time->tm_sec;
}

#include <cstdio>

bool Hardware::Reboot() {
	printf("Rebooting ...\n");
	WatchdogStop();
	
	RebootHandler();

	WatchdogInit();

	SetMode(hardware::ledblink::Mode::REBOOT);

	for (;;) {
		Run();
	}

	__builtin_unreachable();
	return true;
}
