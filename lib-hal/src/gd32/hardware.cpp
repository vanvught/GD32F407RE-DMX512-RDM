/**
 * @file hardware.cpp
 *
 */
/* Copyright (C) 2021-2024 by Arjan van Vught mailto:info@gd32-dmx.org
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
#include <cstdio>
#include <cassert>

#include "hardware.h"
#include "panel_led.h"

#include "gd32.h"
#include "gd32_i2c.h"
#include "gd32_adc.h"
#if defined (CONFIG_ENET_ENABLE_PTP)
# include "gd32_ptp.h"
#endif

#if defined (DEBUG_I2C)
# include "../debug/i2c/i2cdetect.h"
#endif

#if defined (ENABLE_USB_HOST)
void usb_init();
#endif

#include "logic_analyzer.h"

#include "debug.h"

#if (defined (GD32F4XX) || defined (GD32H7XX)) && defined(GPIO_INIT)
# error
#endif

extern "C" {
void systick_config(void);
}

void console_init();
void udelay_init();
void gd32_adc_init();

#if defined (GD32H7XX)
void cache_enable();
void mpu_config();
#endif

namespace hal {
void uuid_init(uuid_t out);
}  // namespace hardware

namespace net {
void net_shutdown();
}  // namespace net

void timer6_config();
#if !defined (CONFIG_ENET_ENABLE_PTP)
# if defined (CONFIG_TIME_USE_TIMER)
#  if defined(GD32H7XX)
void timer16_config();
#  else
void timer7_config();
#  endif
# endif
#endif

Hardware *Hardware::s_pThis;

Hardware::Hardware() {
	assert(s_pThis == nullptr);
	s_pThis = this;

#if defined (GD32H7XX)
    cache_enable();
    mpu_config();
#endif

	console_init();
	timer6_config();
#if defined (CONFIG_HAL_USE_SYSTICK)
	systick_config();
#endif
#if !defined (CONFIG_ENET_ENABLE_PTP)
# if defined (CONFIG_TIME_USE_TIMER)
#  if defined(GD32H7XX)
	timer16_config();
#  else
	timer7_config();
#  endif
# endif
#endif
    udelay_init();
    hal::uuid_init(m_uuid);
	gd32_adc_init();
	gd32_i2c_begin();

	rcu_periph_clock_enable(RCU_TIMER5);

	timer_deinit(TIMER5);

	timer_parameter_struct timer_initpara;
	timer_struct_para_init(&timer_initpara);

	timer_initpara.prescaler = TIMER_PSC_1MHZ;
	timer_initpara.period = static_cast<uint32_t>(~0);
	timer_init(TIMER5, &timer_initpara);
	timer_enable(TIMER5);

#if defined (GD32H7XX)
#elif defined (GD32F4XX)
	rcu_timer_clock_prescaler_config(RCU_TIMER_PSC_MUL4);
#else
#endif

#ifndef NDEBUG
	const auto nSYS = rcu_clock_freq_get(CK_SYS);
	const auto nAHB = rcu_clock_freq_get(CK_AHB);
	const auto nAPB1 = rcu_clock_freq_get(CK_APB1);
	const auto nAPB2 = rcu_clock_freq_get(CK_APB2);
	printf("CK_SYS=%u\nCK_AHB=%u\nCK_APB1=%u\nCK_APB2=%u\n", nSYS, nAHB, nAPB1, nAPB2);
	assert(nSYS == MCU_CLOCK_FREQ);
	assert(nAHB == AHB_CLOCK_FREQ);
	assert(nAPB1 == APB1_CLOCK_FREQ);
	assert(nAPB2 == APB2_CLOCK_FREQ);
# if defined (GD32H7XX)
	const auto nAPB3 = rcu_clock_freq_get(CK_APB3);
	const auto nAPB4 = rcu_clock_freq_get(CK_APB4);
	printf("nCK_APB3=%u\nCK_APB4=%u\n", nAPB3, nAPB4);
	assert(nAPB3 == APB3_CLOCK_FREQ);
	assert(nAPB4 == APB4_CLOCK_FREQ);
# endif
#endif

#if defined (GD32H7XX)
    rcu_periph_clock_enable(RCU_PMU);
    rcu_periph_clock_enable(RCU_BKPSRAM);
    pmu_backup_write_enable();
#elif defined (GD32F4XX)
	rcu_periph_clock_enable(RCU_RTC);
	rcu_periph_clock_enable(RCU_PMU);
	pmu_backup_ldo_config(PMU_BLDOON_ON);
	rcu_periph_clock_enable(RCU_BKPSRAM);
	pmu_backup_write_enable();
#else
	rcu_periph_clock_enable (RCU_BKPI);
	rcu_periph_clock_enable (RCU_PMU);
	pmu_backup_write_enable();
#endif
	bkp_data_write(BKP_DATA_1, 0x0);

#if !defined (ENABLE_TFTP_SERVER)
# if defined (GD32F207RG) || defined (GD32F4XX) || defined (GD32H7XX)
#  if !defined (GD32H7XX)
	// clear section .dmx
	extern unsigned char _sdmx;
	extern unsigned char _edmx;
	DEBUG_PRINTF("clearing .dmx at %p, size %u", &_sdmx, &_edmx - &_sdmx);
	memset(&_sdmx, 0, &_edmx - &_sdmx);
#  endif
#  if defined (GD32F450VI) || defined (GD32H7XX)
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
#  if !defined (GD32F450VE) && !defined (GD32H7XX)
	// clear section .pixel
	extern unsigned char _spixel;
	extern unsigned char _epixel;
	DEBUG_PRINTF("clearing .pixel at %p, size %u", &_spixel, &_epixel - &_spixel);
	memset(&_spixel, 0, &_epixel - &_spixel);
#  endif
# endif
#else
# if defined (GD32F20X) || defined (GD32F4XX) || defined (GD32H7XX)
	// clear section .network
	extern unsigned char _snetwork;
	extern unsigned char _enetwork;
	DEBUG_PRINTF("clearing .network at %p, size %u", &_snetwork, &_enetwork - &_snetwork);
	memset(&_snetwork, 0, &_enetwork - &_snetwork);
# endif
#endif

#if !defined (CONFIG_ENET_ENABLE_PTP)
	struct tm tmbuf;
	memset(&tmbuf, 0, sizeof(struct tm));
	tmbuf.tm_mday = _TIME_STAMP_DAY_;			// The day of the month, in the range 1 to 31.
	tmbuf.tm_mon = _TIME_STAMP_MONTH_ - 1;		// The number of months since January, in the range 0 to 11.
	tmbuf.tm_year = _TIME_STAMP_YEAR_ - 1900;	// The number of years since 1900.

	const auto seconds = mktime(&tmbuf);
	const struct timeval tv = { seconds, 0 };

	settimeofday(&tv, nullptr);
#endif

#if !defined(DISABLE_RTC)
	m_HwClock.RtcProbe();
	m_HwClock.Print();
# if !defined (CONFIG_ENET_ENABLE_PTP)
	// Set the System Clock from the Hardware Clock
	m_HwClock.HcToSys();
# endif
#endif

#if !defined(CONFIG_LEDBLINK_USE_PANELLED)
	rcu_periph_clock_enable(LED_BLINK_GPIO_CLK);
# if defined (GPIO_INIT)
	gpio_init(LED_BLINK_GPIO_PORT, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, LED_BLINK_PIN);
# else
	gpio_mode_set(LED_BLINK_GPIO_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, LED_BLINK_PIN);
	gpio_output_options_set(LED_BLINK_GPIO_PORT, GPIO_OTYPE_PP, GPIO_OSPEED, LED_BLINK_PIN);
# endif
	GPIO_BOP(LED_BLINK_GPIO_PORT) = LED_BLINK_PIN;
#endif

#if defined (PANELLED_595_CS_GPIOx)
	rcu_periph_clock_enable(PANELLED_595_CS_RCU_GPIOx);
# if defined (GPIO_INIT)
	gpio_init(PANELLED_595_CS_GPIOx, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, PANELLED_595_CS_GPIO_PINx);
# else
	gpio_mode_set(PANELLED_595_CS_GPIOx, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, PANELLED_595_CS_GPIO_PINx);
	gpio_output_options_set(PANELLED_595_CS_GPIOx, GPIO_OTYPE_PP, GPIO_OSPEED, PANELLED_595_CS_GPIO_PINx);
# endif
	GPIO_BOP(PANELLED_595_CS_GPIOx) = PANELLED_595_CS_GPIO_PINx;
#endif

	hal::panel_led_init();

#if defined ENABLE_USB_HOST
	usb_init();
#endif

	logic_analyzer::init();

#if defined (DEBUG_I2C)
	I2cDetect i2cdetect;
#endif

	SetFrequency(1U);
}

bool Hardware::Reboot() {
	puts("Rebooting ...");
	
#if !defined(DISABLE_RTC)
	m_HwClock.SysToHc();
#endif

	WatchdogStop();

	RebootHandler();

	net::net_shutdown();

	SetMode(hardware::ledblink::Mode::OFF_OFF);

	NVIC_SystemReset();

	__builtin_unreachable();
	return true;
}
