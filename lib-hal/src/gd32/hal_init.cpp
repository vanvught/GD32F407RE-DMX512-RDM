/**
 * @file hal_init.cpp
 *
 */
/* Copyright (C) 2025 by Arjan van Vught mailto:info@gd32-dmx.org
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

#if defined(DEBUG_HAL)
#undef NDEBUG
#endif

#if (defined(GD32F4XX) || defined(GD32H7XX)) && defined(GPIO_INIT)
#error
#endif

#include <cstdint>
#include <cstring>
#include <cstdio>
#include <cassert>
#include <ctime>
#include <sys/time.h>

#include "gd32.h"
#include "gd32_i2c.h"
#include "gd32_adc.h"
#if defined(CONFIG_NET_ENABLE_PTP)
#include "gd32_ptp.h"
#endif

#if defined(DEBUG_I2C)
#include "../debug/i2c/i2cdetect.h"
#endif

#include "hal_statusled.h"
#include "hal_panelled.h"
#include "logic_analyzer.h"

#include "firmware/debug/debug_debug.h"

#if defined(ENABLE_USB_HOST)
void usb_init();
#endif

#if defined(CONFIG_HAL_USE_SYSTICK)
void SystickConfig();
#endif

namespace console {
void Init();
}

void UdelayInit();
void Gd32AdcInit();

#if defined(GD32H7XX)
void CacheEnable();
void MpuConfig();
#endif

void Timer5Config();
void Timer6Config();
#if !defined(CONFIG_NET_ENABLE_PTP)
#if defined(CONFIG_TIME_USE_TIMER)
#if defined(GD32H7XX)
void Timer16Config();
#else
void Timer7Config();
#endif
#endif
#endif

#if !defined(DISABLE_RTC)
#include "hwclock.h"
static HwClock hwClock;
#endif

extern unsigned char _sdmx;
extern unsigned char _edmx;
extern unsigned char _slightset;
extern unsigned char _elightset;
extern unsigned char _snetwork;
extern unsigned char _enetwork;
extern unsigned char _spixel;
extern unsigned char _epixel;

namespace hal
{
namespace global
{
bool watchdog = false;
}
void Init()
{
    /*
     * GD32H7xx Cache and Memory Protection Unit
     */

#if defined(GD32H7XX)
    CacheEnable();
    MpuConfig();
#endif

    console::Init();

    /*
     * From here we console output
     */

    /*
     * See https://www.gd32-dmx.org/memory.html
     */

#ifndef NDEBUG
    putchar('\n');
#endif
#if !defined(ENABLE_TFTP_SERVER)
#if defined(GD32F207RG) || defined(GD32F4XX) || defined(GD32H7XX)
#if !defined(GD32H7XX)
    {
        // Clear section .dmx
        const auto kSize = (&_edmx - &_sdmx);
        memset(&_sdmx, 0, kSize);
#ifndef NDEBUG
        printf("Cleared .dmx at %p, size %u\n", &_sdmx, kSize);
#endif
    }
#endif
#if defined(GD32F450VI) || defined(GD32H7XX)
    {
        // Clear section .lightset
        const auto kSize = (&_elightset - &_slightset);
        memset(&_slightset, 0, kSize);
#ifndef NDEBUG
        printf("Cleared .lightset at %p, size %u\n", &_slightset, kSize);
#endif
    }
#endif
    {
        // Clear section .network
        const auto kSize = (&_enetwork - &_snetwork);
        memset(&_snetwork, 0, kSize);
#ifndef NDEBUG
        printf("Cleared .network at %p, size %u\n", &_snetwork, kSize);
#endif
    }
#if !defined(GD32F450VE) && !defined(GD32H7XX)
    {
        // Clear section .pixel
        const auto kSize = (&_epixel - &_spixel);
        memset(&_spixel, 0, kSize);
#ifndef NDEBUG
        printf("Cleared .pixel at %p, size %u\n", &_spixel, kSize);
#endif
    }
#endif
#endif
#else
#if defined(GD32F20X) || defined(GD32F4XX) || defined(GD32H7XX)
    {
        // clear section .network
        const auto kSize = (&_enetwork - &_snetwork);
        memset(&_snetwork, 0, kSize);
#ifndef NDEBUG
        printf("Cleared .network at %p, size %u\n", &_snetwork, kSize);
#endif
    }
#endif
#endif

    /*
     * Show the AHB and APBx busses frequency
     */

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
#if defined(GD32H7XX)
    const auto nAPB3 = rcu_clock_freq_get(CK_APB3);
    const auto nAPB4 = rcu_clock_freq_get(CK_APB4);
    printf("nCK_APB3=%u\nCK_APB4=%u\n", nAPB3, nAPB4);
    assert(nAPB3 == APB3_CLOCK_FREQ);
    assert(nAPB4 == APB4_CLOCK_FREQ);
#endif
#endif

    /*
     * Setup the TIMERx
     */

#if defined(GD32H7XX)
#elif defined(GD32F4XX)
    /*
     * AHB = SYSCLK = 240 MHz (GD32F470), others = 200 MHz
     * APB1 = AHB / 4 =   50 MHz => APB1PSC = 0b101
     * APB2 = AHB / 2  = 100 MHz => APB2PSC = 0b100
     */

    rcu_timer_clock_prescaler_config(RCU_TIMER_PSC_MUL4);

    /*
     * If APB1PSC/APB2PSC in RCU_CFG0 register is 0b0xx(CK_APBx = CK_AHB),
     * 0b100(CK_APBx = CK_AHB/2), or 0b101(CK_APBx = CK_AHB/4), the TIMER
     * clock is equal to CK_AHB(CK_TIMERx = CK_AHB).
     */

    /*
     * TIMER in APB1 domain: CK_TIMERx = AHB = 200 MHz => 240 MHz (GD32F470).
     * TIMER in APB2 domain: CK_TIMERx = AHB = 200 MHz => 240 MHz (GD32F470).
     */
#else
#endif

    Timer5Config();
    Timer6Config();
#if defined(CONFIG_HAL_USE_SYSTICK)
    SystickConfig();
#endif
#if !defined(CONFIG_NET_ENABLE_PTP)
#if defined(CONFIG_TIME_USE_TIMER)
#if defined(GD32H7XX)
    Timer16Config();
#else
    Timer7Config();
#endif
#endif
#endif

    UdelayInit();
    Gd32AdcInit();
    Gd32I2cBegin();
#if defined(CONFIG_ENABLE_I2C1)
    Gd32I2c1Begin();
#endif

#if defined(GD32H7XX)
    rcu_periph_clock_enable(RCU_PMU);
    rcu_periph_clock_enable(RCU_BKPSRAM);
    pmu_backup_write_enable();
#elif defined(GD32F4XX)
    rcu_periph_clock_enable(RCU_RTC);
    rcu_periph_clock_enable(RCU_PMU);
    pmu_backup_ldo_config(PMU_BLDOON_ON);
    rcu_periph_clock_enable(RCU_BKPSRAM);
    pmu_backup_write_enable();
#else
    rcu_periph_clock_enable(RCU_BKPI);
    rcu_periph_clock_enable(RCU_PMU);
    pmu_backup_write_enable();
#endif
    bkp_data_write(BKP_DATA_1, 0x0);

#if defined(CONFIG_HAVE_CRC32_HW)
    rcu_periph_clock_enable(RCU_CRC);
    crc_data_register_reset();
#endif

    /*
     * Initialize status led, 74hc595 and panel led
     */

#if !defined(CONFIG_LEDBLINK_USE_PANELLED)
    rcu_periph_clock_enable(LED_BLINK_GPIO_CLK);
#if defined(GPIO_INIT)
    gpio_init(LED_BLINK_GPIO_PORT, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, LED_BLINK_PIN);
#else
    gpio_mode_set(LED_BLINK_GPIO_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, LED_BLINK_PIN);
    gpio_output_options_set(LED_BLINK_GPIO_PORT, GPIO_OTYPE_PP, GPIO_OSPEED, LED_BLINK_PIN);
#endif
    GPIO_BOP(LED_BLINK_GPIO_PORT) = LED_BLINK_PIN;
#endif

#if defined(PANELLED_595_CS_GPIOx)
    rcu_periph_clock_enable(PANELLED_595_CS_RCU_GPIOx);
#if defined(GPIO_INIT)
    gpio_init(PANELLED_595_CS_GPIOx, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, PANELLED_595_CS_GPIO_PINx);
#else
    gpio_mode_set(PANELLED_595_CS_GPIOx, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, PANELLED_595_CS_GPIO_PINx);
    gpio_output_options_set(PANELLED_595_CS_GPIOx, GPIO_OTYPE_PP, GPIO_OSPEED, PANELLED_595_CS_GPIO_PINx);
#endif
    GPIO_BOP(PANELLED_595_CS_GPIOx) = PANELLED_595_CS_GPIO_PINx;
#endif

    hal::panelled::Init();

#if defined ENABLE_USB_HOST
    usb_init();
#endif

    logic_analyzer::Init();

#if !defined(CONFIG_NET_ENABLE_PTP)
    struct tm tmbuf;
    memset(&tmbuf, 0, sizeof(struct tm));
    tmbuf.tm_mday = _TIME_STAMP_DAY_;         // The day of the month, in the range 1 to 31.
    tmbuf.tm_mon = _TIME_STAMP_MONTH_ - 1;    // The number of months since January, in the range 0 to 11.
    tmbuf.tm_year = _TIME_STAMP_YEAR_ - 1900; // The number of years since 1900.

    const auto kSeconds = mktime(&tmbuf);
    const struct timeval kTv = {kSeconds, 0};

    settimeofday(&kTv, nullptr);
#endif

#if !defined(DISABLE_RTC)
    HwClock::Get()->RtcProbe();
    HwClock::Get()->Print();
#if !defined(CONFIG_NET_ENABLE_PTP)
    // Set the System Clock from the Hardware Clock
    HwClock::Get()->HcToSys();
#endif
#endif

#if defined(DEBUG_I2C)
    I2cDetect();
#if defined(CONFIG_ENABLE_I2C1)
    I2c1Detect();
#endif
#endif

#if !defined(USE_FREE_RTOS)
    hal::statusled::SetFrequency(1);
#endif
}
} // namespace hal
