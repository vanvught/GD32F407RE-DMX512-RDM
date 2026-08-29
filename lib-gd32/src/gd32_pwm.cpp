/**
 * @file gd32_pwm.cpp
 *
 */
/* Copyright (C) 2024-2026 by Arjan van Vught mailto:info@gd32-dmx.org
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
#include <cstdio>

#include "gd32_pwm.h"
#include "gd32.h"
#include "gd32_debug.h"

#if defined(PWM_RCU_TIMERx) && defined(PWM_TIMERx)
namespace pwm {
#if !defined(PWM_CHANNEL_0_DUTYCYCLE)
#define PWM_CHANNEL_0_DUTYCYCLE 50
#endif // PWM_CHANNEL_0_DUTYCYCLE
#if !defined(PWM_CHANNEL_1_DUTYCYCLE)
#define PWM_CHANNEL_1_DUTYCYCLE 50
#endif // PWM_CHANNEL_1_DUTYCYCLE
#if !defined(PWM_CHANNEL_2_DUTYCYCLE)
#define PWM_CHANNEL_2_DUTYCYCLE 50
#endif // PWM_CHANNEL_2_DUTYCYCLE
#if !defined(PWM_CHANNEL_3_DUTYCYCLE)
#define PWM_CHANNEL_3_DUTYCYCLE 50
#endif // PWM_CHANNEL_3_DUTYCYCLE
#if defined(PWM_CH0_RCU_GPIOx)
static constexpr uint32_t kDefaulChannel0Dutycycle = PWM_CHANNEL_0_DUTYCYCLE;
#endif // PWM_CH0_RCU_GPIOx
#if defined(PWM_CH1_RCU_GPIOx)
static constexpr uint32_t kDefaulChannel1Dutycycle = PWM_CHANNEL_1_DUTYCYCLE;
#endif // PWM_CH1_RCU_GPIOx
#if defined(PWM_CH2_RCU_GPIOx)
static constexpr uint32_t kDefaulChannel2Dutycycle = PWM_CHANNEL_2_DUTYCYCLE;
#endif // PWM_CH2_RCU_GPIOx
#if defined(PWM_CH3_RCU_GPIOx)
static constexpr uint32_t kDefaulChannel3Dutycycle = PWM_CHANNEL_3_DUTYCYCLE;
#endif // PWM_CH3_RCU_GPIOx
#if defined(PWM_RCU_TIMERx) && defined(PWM_TIMERx)
static constexpr uint32_t kTimerPeriod = 19999; // 50KHz
#endif // defined(PWM_RCU_TIMERx) && defined(PWM_TIMERx)
} // namespace pwm

static void Dump() {
#ifdef DEBUG_GD32_PWM
    GD32_PWM_DEBUG_ENTRY();
#ifndef NDEBUG
    printf("PWM_TIMERx=0x%.8X\n", PWM_TIMERx - TIMER_BASE);
    printf("PWM_RCU_TIMERx=0x%.8X\n", PWM_RCU_TIMERx);
#if defined(GPIO_INIT)
    printf("PWM_TIMER_REMAP=0x%.8X\n", PWM_TIMER_REMAP);
#endif // GPIO_INIT
    puts("--------------------------------");
#if defined(PWM_CH0_RCU_GPIOx)
    printf("PWM_CH0_RCU_GPIOx=0x%.8X\n", PWM_CH0_RCU_GPIOx);
    printf("PWM_CH0_GPIOx=0x%.8X\n", PWM_CH0_GPIOx);
    printf("PWM_CH0_GPIO_PINx=0x%.4X\n", PWM_CH0_GPIO_PINx);
    puts("--------------------------------");
#endif // PWM_CH0_RCU_GPIOx
#if defined(PWM_CH1_RCU_GPIOx)
    printf("PWM_CH1_RCU_GPIOx=0x%.8X\n", PWM_CH1_RCU_GPIOx);
    printf("PWM_CH1_GPIOx=0x%.8X\n", PWM_CH1_GPIOx);
    printf("PWM_CH1_GPIO_PINx=0x%.4X\n", PWM_CH1_GPIO_PINx);
    puts("--------------------------------");
#endif // PWM_CH1_RCU_GPIOx
#if defined(PWM_CH2_RCU_GPIOx)
    printf("PWM_CH2_RCU_GPIOx=0x%.8X\n", PWM_CH2_RCU_GPIOx);
    printf("PWM_CH2_GPIOx=0x%.8X\n", PWM_CH2_GPIOx);
    printf("PWM_CH2_GPIO_PINx=0x%.4X\n", PWM_CH2_GPIO_PINx);
    puts("--------------------------------");
#endif // PWM_CH2_RCU_GPIOx
#if defined(PWM_CH3_RCU_GPIOx)
    printf("PWM_CH3_RCU_GPIOx=0x%.8X\n", PWM_CH3_RCU_GPIOx);
    printf("PWM_CH3_GPIOx=0x%.8X\n", PWM_CH3_GPIOx);
    printf("PWM_CH3_GPIO_PINx=0x%.4X\n", PWM_CH3_GPIO_PINx);
#endif // PWM_CH3_RCU_GPIOx
#endif // NDEBUG
    GD32_PWM_DEBUG_EXIT();
#endif // DEBUG_GD32_PWM
}

static void GpioConfig() {
#if defined(PWM_CH0_RCU_GPIOx)
    rcu_periph_clock_enable(PWM_CH0_RCU_GPIOx);
#endif // PWM_CH0_RCU_GPIOx
#if defined(PWM_CH1_RCU_GPIOx)
    rcu_periph_clock_enable(PWM_CH1_RCU_GPIOx);
#endif // PWM_CH1_RCU_GPIOx
#if defined(PWM_CH2_RCU_GPIOx)
    rcu_periph_clock_enable(PWM_CH2_RCU_GPIOx);
#endif // PWM_CH2_RCU_GPIOx
#if defined(PWM_CH3_RCU_GPIOx)
    rcu_periph_clock_enable(PWM_CH3_RCU_GPIOx);
#endif // PWM_CH3_RCU_GPIOx

#if defined(GPIO_INIT)
    rcu_periph_clock_enable(RCU_AF);
#if defined(PWM_TIMER_REMAP)
    gpio_pin_remap_config(PWM_TIMER_REMAP, ENABLE);
#endif // PWM_TIMER_REMAP
#if defined(PWM_CH0_GPIOx) && defined(PWM_CH0_GPIO_PINx)
    gpio_init(PWM_CH0_GPIOx, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, PWM_CH0_GPIO_PINx);
#endif // defined(PWM_CH0_GPIOx) && defined(PWM_CH0_GPIO_PINx)
#if defined(PWM_CH1_GPIOx) && defined(PWM_CH1_GPIO_PINx)
    gpio_init(PWM_CH1_GPIOx, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, PWM_CH1_GPIO_PINx);
#endif // defined(PWM_CH1_GPIOx) && defined(PWM_CH1_GPIO_PINx)
#if defined(PWM_CH2_GPIOx) && defined(PWM_CH2_GPIO_PINx)
    gpio_init(PWM_CH2_GPIOx, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, PWM_CH2_GPIO_PINx);
#endif // defined(PWM_CH2_GPIOx) && defined(PWM_CH2_GPIO_PINx)
#if defined(PWM_CH3_GPIOx) && defined(PWM_CH3_GPIO_PINx)
    gpio_init(PWM_CH3_GPIOx, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, PWM_CH3_GPIO_PINx);
#endif // defined(PWM_CH3_GPIOx) && defined(PWM_CH3_GPIO_PINx)
#else
#if defined(PWM_CH0_GPIOx) && defined(PWM_CH0_GPIO_PINx)
    gpio_af_set(PWM_CH0_GPIOx, PWM_GPIO_AFx, PWM_CH0_GPIO_PINx);
    gpio_mode_set(PWM_CH0_GPIOx, GPIO_MODE_AF, GPIO_PUPD_NONE, PWM_CH0_GPIO_PINx);
    gpio_output_options_set(PWM_CH0_GPIOx, GPIO_OTYPE_PP, GPIO_OSPEED, PWM_CH0_GPIO_PINx);
#endif // defined(PWM_CH0_GPIOx) && defined(PWM_CH0_GPIO_PINx)
#if defined(PWM_CH1_GPIOx) && defined(PWM_CH1_GPIO_PINx)
    gpio_af_set(PWM_CH1_GPIOx, PWM_GPIO_AFx, PWM_CH1_GPIO_PINx);
    gpio_mode_set(PWM_CH1_GPIOx, GPIO_MODE_AF, GPIO_PUPD_NONE, PWM_CH1_GPIO_PINx);
    gpio_output_options_set(PWM_CH1_GPIOx, GPIO_OTYPE_PP, GPIO_OSPEED, PWM_CH1_GPIO_PINx);
#endif // defined(PWM_CH1_GPIOx) && defined(PWM_CH1_GPIO_PINx)
#if defined(PWM_CH2_GPIOx) && defined(PWM_CH2_GPIO_PINx)
    gpio_af_set(PWM_CH2_GPIOx, PWM_GPIO_AFx, PWM_CH2_GPIO_PINx);
    gpio_mode_set(PWM_CH2_GPIOx, GPIO_MODE_AF, GPIO_PUPD_NONE, PWM_CH2_GPIO_PINx);
    gpio_output_options_set(PWM_CH2_GPIOx, GPIO_OTYPE_PP, GPIO_OSPEED, PWM_CH2_GPIO_PINx);
#endif // defined(PWM_CH2_GPIOx) && defined(PWM_CH2_GPIO_PINx)
#if defined(PWM_CH3_GPIOx) && defined(PWM_CH3_GPIO_PINx)
    gpio_af_set(PWM_CH3_GPIOx, PWM_GPIO_AFx, PWM_CH3_GPIO_PINx);
    gpio_mode_set(PWM_CH3_GPIOx, GPIO_MODE_AF, GPIO_PUPD_NONE, PWM_CH3_GPIO_PINx);
    gpio_output_options_set(PWM_CH3_GPIOx, GPIO_OTYPE_PP, GPIO_OSPEED, PWM_CH3_GPIO_PINx);
#endif // defined(PWM_CH3_GPIOx) && defined(PWM_CH3_GPIO_PINx)
#endif // GPIO_INIT
}

static void TimerConfig() {
    rcu_periph_clock_enable(PWM_RCU_TIMERx);

    timer_deinit(PWM_TIMERx);

    timer_parameter_struct timer_initpara;
    timer_initpara.prescaler = TIMER_PSC_1MHZ;
    timer_initpara.alignedmode = TIMER_COUNTER_EDGE;
    timer_initpara.counterdirection = TIMER_COUNTER_UP;
    timer_initpara.period = pwm::kTimerPeriod;
    timer_initpara.clockdivision = TIMER_CKDIV_DIV1;
    timer_initpara.repetitioncounter = 0;
    timer_init(PWM_TIMERx, &timer_initpara);

#if defined(PWM_CH0_RCU_GPIOx) || defined(PWM_CH1_RCU_GPIOx) || defined(PWM_CH2_RCU_GPIOx) || defined(PWM_CH3_RCU_GPIOx)
    timer_oc_parameter_struct timer_ocintpara;
    timer_ocintpara.ocpolarity = TIMER_OC_POLARITY_HIGH;
    timer_ocintpara.outputstate = TIMER_CCX_ENABLE;
    timer_ocintpara.ocnpolarity = TIMER_OCN_POLARITY_HIGH;
    timer_ocintpara.outputnstate = TIMER_CCXN_DISABLE;
    timer_ocintpara.ocidlestate = TIMER_OC_IDLE_STATE_LOW;
    timer_ocintpara.ocnidlestate = TIMER_OCN_IDLE_STATE_LOW;
#endif // defined(PWM_CH0_RCU_GPIOx) || defined(PWM_CH1_RCU_GPIOx) || defined(PWM_CH2_RCU_GPIOx) || defined(PWM_CH3_RCU_GPIOx)
#if defined(PWM_CH0_RCU_GPIOx)
    timer_channel_output_config(PWM_TIMERx, TIMER_CH_0, &timer_ocintpara);
    timer_channel_output_pulse_value_config(PWM_TIMERx, TIMER_CH_0, 1999);
    timer_channel_output_mode_config(PWM_TIMERx, TIMER_CH_0, TIMER_OC_MODE_PWM0);
    timer_channel_output_shadow_config(PWM_TIMERx, TIMER_CH_0, TIMER_OC_SHADOW_DISABLE);
#endif // PWM_CH0_RCU_GPIOx
#if defined(PWM_CH1_RCU_GPIOx)
    timer_channel_output_config(PWM_TIMERx, TIMER_CH_1, &timer_ocintpara);
    timer_channel_output_pulse_value_config(PWM_TIMERx, TIMER_CH_1, 3999);
    timer_channel_output_mode_config(PWM_TIMERx, TIMER_CH_1, TIMER_OC_MODE_PWM0);
    timer_channel_output_shadow_config(PWM_TIMERx, TIMER_CH_1, TIMER_OC_SHADOW_DISABLE);
#endif // PWM_CH1_RCU_GPIOx
#if defined(PWM_CH2_RCU_GPIOx)
    timer_channel_output_config(PWM_TIMERx, TIMER_CH_2, &timer_ocintpara);
    timer_channel_output_pulse_value_config(PWM_TIMERx, TIMER_CH_2, 7999);
    timer_channel_output_mode_config(PWM_TIMERx, TIMER_CH_2, TIMER_OC_MODE_PWM0);
    timer_channel_output_shadow_config(PWM_TIMERx, TIMER_CH_2, TIMER_OC_SHADOW_DISABLE);
#endif // PWM_CH2_RCU_GPIOx
#if defined(PWM_CH3_RCU_GPIOx)
    timer_channel_output_config(PWM_TIMERx, TIMER_CH_3, &timer_ocintpara);
    timer_channel_output_pulse_value_config(PWM_TIMERx, TIMER_CH_3, 11999);
    timer_channel_output_mode_config(PWM_TIMERx, TIMER_CH_3, TIMER_OC_MODE_PWM0);
    timer_channel_output_shadow_config(PWM_TIMERx, TIMER_CH_3, TIMER_OC_SHADOW_DISABLE);
#endif // PWM_CH3_RCU_GPIOx

    timer_auto_reload_shadow_enable(PWM_TIMERx);
    timer_enable(PWM_TIMERx);
}

void gd32_pwm_begin() {
    GD32_PWM_DEBUG_ENTRY();

    Dump();

    GpioConfig();
    TimerConfig();

#if defined(PWM_CH0_RCU_GPIOx)
    gd32_pwm_set_duty_cycle(pwm::Channel::PWM_CHANNEL_0, pwm::kDefaulChannel0Dutycycle);
#endif // PWM_CH0_RCU_GPIOx
#if defined(PWM_CH1_RCU_GPIOx)
    gd32_pwm_set_duty_cycle(pwm::Channel::PWM_CHANNEL_1, pwm::kDefaulChannel1Dutycycle);
#endif // PWM_CH1_RCU_GPIOx
#if defined(PWM_CH2_RCU_GPIOx)
    gd32_pwm_set_duty_cycle(pwm::Channel::PWM_CHANNEL_2, pwm::kDefaulChannel2Dutycycle);
#endif // PWM_CH2_RCU_GPIOx
#if defined(PWM_CH3_RCU_GPIOx)
    gd32_pwm_set_duty_cycle(pwm::Channel::PWM_CHANNEL_3, pwm::kDefaulChannel3Dutycycle);
#endif // PWM_CH3_RCU_GPIOx

    GD32_PWM_DEBUG_EXIT();
}

void gd32_pwm_set_duty_cycle(pwm::Channel channel, uint32_t duty_cycle) {
    GD32_PWM_DEBUG_ENTRY();

    const uint32_t kPulse = (duty_cycle > 100U ? 100U : duty_cycle) * (pwm::kTimerPeriod / 100U);

    DEBUG_PRINTF("channel=%u, duty_cycle=%u, kPulse=%u", static_cast<unsigned>(channel), duty_cycle, static_cast<unsigned>(kPulse));

    switch (channel) {
#if defined(PWM_CH0_RCU_GPIOx)
        case pwm::Channel::PWM_CHANNEL_0:
            timer_channel_output_pulse_value_config(PWM_TIMERx, TIMER_CH_0, kPulse);
            break;
#endif // PWM_CH0_RCU_GPIOx
#if defined(PWM_CH1_RCU_GPIOx)
        case pwm::Channel::PWM_CHANNEL_1:
            timer_channel_output_pulse_value_config(PWM_TIMERx, TIMER_CH_1, kPulse);
            break;
#endif // PWM_CH1_RCU_GPIOx
#if defined(PWM_CH2_RCU_GPIOx)
        case pwm::Channel::PWM_CHANNEL_2:
            timer_channel_output_pulse_value_config(PWM_TIMERx, TIMER_CH_2, kPulse);
            break;
#endif // PWM_CH2_RCU_GPIOx
#if defined(PWM_CH3_RCU_GPIOx)
        case pwm::Channel::PWM_CHANNEL_3:
            timer_channel_output_pulse_value_config(PWM_TIMERx, TIMER_CH_3, kPulse);
            break;
#endif // PWM_CH3_RCU_GPIOx
        default:
            break;
    }

    GD32_PWM_DEBUG_EXIT();
}
#endif // defined(PWM_RCU_TIMERx) && defined(PWM_TIMERx)