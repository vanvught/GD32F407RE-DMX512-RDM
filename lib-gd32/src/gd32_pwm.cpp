/**
 * @file gd32_pwm.cpp
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

#if 0
#undef NDEBUG

#define PWM_TIMERx				TIMER2
#define PWM_RCU_TIMERx			RCU_TIMER2

#define TIMER2_FULL_REMAP
#define PWM_TIMER_REMAP			TIMER2_REMAP
#define PWM_GPIO_AFx			TIMER2_GPIO_AFx

#define PWM_CH0_RCU_GPIOx 		TIMER2_CH0_RCU_GPIOx
#define PWM_CH0_GPIOx			TIMER2_CH0_GPIOx
#define PWM_CH0_GPIO_PINx		TIMER2_CH0_GPIO_PINx

#define PWM_CH1_RCU_GPIOx 		TIMER2_CH1_RCU_GPIOx
#define PWM_CH1_GPIOx			TIMER2_CH1_GPIOx
#define PWM_CH1_GPIO_PINx		TIMER2_CH1_GPIO_PINx

#define PWM_CH2_RCU_GPIOx 		TIMER2_CH2_RCU_GPIOx
#define PWM_CH2_GPIOx			TIMER2_CH2_GPIOx
#define PWM_CH2_GPIO_PINx		TIMER2_CH2_GPIO_PINx

#define PWM_CH3_RCU_GPIOx 		TIMER2_CH3_RCU_GPIOx
#define PWM_CH3_GPIOx			TIMER2_CH3_GPIOx
#define PWM_CH3_GPIO_PINx		TIMER2_CH3_GPIO_PINx
#endif

#include <cstdint>
#ifndef NDEBUG
# include <cstdio>
#endif

#include "gd32_pwm.h"
#include "gd32.h"

#include "debug.h"

namespace pwm {
#if !defined(PWM_CHANNEL_0_DUTYCYCLE)
# define PWM_CHANNEL_0_DUTYCYCLE 50
#endif
#if !defined(PWM_CHANNEL_1_DUTYCYCLE)
# define PWM_CHANNEL_1_DUTYCYCLE 50
#endif
#if !defined(PWM_CHANNEL_2_DUTYCYCLE)
# define PWM_CHANNEL_2_DUTYCYCLE 50
#endif
#if !defined(PWM_CHANNEL_3_DUTYCYCLE)
# define PWM_CHANNEL_3_DUTYCYCLE 50
#endif
#if defined (PWM_CH0_RCU_GPIOx)
static constexpr uint32_t DEFAUL_CHANNEL_0_DUTYCYCLE = PWM_CHANNEL_0_DUTYCYCLE;
#endif
#if defined (PWM_CH1_RCU_GPIOx)
static constexpr uint32_t DEFAUL_CHANNEL_1_DUTYCYCLE = PWM_CHANNEL_1_DUTYCYCLE;
#endif
#if defined (PWM_CH2_RCU_GPIOx)
static constexpr uint32_t DEFAUL_CHANNEL_2_DUTYCYCLE = PWM_CHANNEL_2_DUTYCYCLE;
#endif
#if defined (PWM_CH3_RCU_GPIOx)
static constexpr uint32_t DEFAUL_CHANNEL_3_DUTYCYCLE = PWM_CHANNEL_3_DUTYCYCLE;
#endif
#if defined (PWM_RCU_TIMERx) && defined (PWM_TIMERx)
static constexpr uint32_t TIMER_PERIOD = 19999; // 50KHz
#endif
}  // namespace pwm

#if defined (PWM_RCU_TIMERx) && defined (PWM_TIMERx)

static void dump() {
#if 1
	 DEBUG_ENTRY
#ifndef NDEBUG
	 printf("PWM_TIMERx=0x%.8X\n", PWM_TIMERx - TIMER_BASE);
	 printf("PWM_RCU_TIMERx=0x%.8X\n", PWM_RCU_TIMERx);
# if defined (GPIO_INIT)
	 printf("PWM_TIMER_REMAP=0x%.8X\n", PWM_TIMER_REMAP);
# endif
	 puts("--------------------------------");
# if defined (PWM_CH0_RCU_GPIOx)
	 printf("PWM_CH0_RCU_GPIOx=0x%.8X\n", PWM_CH0_RCU_GPIOx);
	 printf("PWM_CH0_GPIOx=0x%.8X\n", PWM_CH0_GPIOx);
	 printf("PWM_CH0_GPIO_PINx=0x%.4X\n", PWM_CH0_GPIO_PINx);
	 puts("--------------------------------");
# endif
# if defined (PWM_CH1_RCU_GPIOx)
	 printf("PWM_CH1_RCU_GPIOx=0x%.8X\n", PWM_CH1_RCU_GPIOx);
	 printf("PWM_CH1_GPIOx=0x%.8X\n", PWM_CH1_GPIOx);
	 printf("PWM_CH1_GPIO_PINx=0x%.4X\n", PWM_CH1_GPIO_PINx);
	 puts("--------------------------------");
# endif
# if defined (PWM_CH2_RCU_GPIOx)
	 printf("PWM_CH2_RCU_GPIOx=0x%.8X\n", PWM_CH2_RCU_GPIOx);
	 printf("PWM_CH2_GPIOx=0x%.8X\n", PWM_CH2_GPIOx);
	 printf("PWM_CH2_GPIO_PINx=0x%.4X\n", PWM_CH2_GPIO_PINx);
	 puts("--------------------------------");
# endif
# if defined (PWM_CH3_RCU_GPIOx)
	 printf("PWM_CH3_RCU_GPIOx=0x%.8X\n", PWM_CH3_RCU_GPIOx);
	 printf("PWM_CH3_GPIOx=0x%.8X\n", PWM_CH3_GPIOx);
	 printf("PWM_CH3_GPIO_PINx=0x%.4X\n", PWM_CH3_GPIO_PINx);
# endif
#endif
	 DEBUG_EXIT
#endif
}

static void gpio_config() {
#if defined (PWM_CH0_RCU_GPIOx)
	rcu_periph_clock_enable(PWM_CH0_RCU_GPIOx);
#endif
#if defined (PWM_CH1_RCU_GPIOx)
	rcu_periph_clock_enable(PWM_CH1_RCU_GPIOx);
#endif
#if defined (PWM_CH2_RCU_GPIOx)
	rcu_periph_clock_enable(PWM_CH2_RCU_GPIOx);
#endif
#if defined (PWM_CH3_RCU_GPIOx)
	rcu_periph_clock_enable(PWM_CH3_RCU_GPIOx);
#endif

#if defined (GPIO_INIT)
	rcu_periph_clock_enable(RCU_AF);
# if defined(PWM_TIMER_REMAP)
	gpio_pin_remap_config(PWM_TIMER_REMAP, ENABLE);
# endif
# if defined (PWM_CH0_GPIOx) && defined (PWM_CH0_GPIO_PINx)
	gpio_init(PWM_CH0_GPIOx, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, PWM_CH0_GPIO_PINx);
# endif
# if defined (PWM_CH1_GPIOx) && defined (PWM_CH1_GPIO_PINx)
	gpio_init(PWM_CH1_GPIOx, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, PWM_CH1_GPIO_PINx);
# endif
# if defined (PWM_CH2_GPIOx) && defined (PWM_CH2_GPIO_PINx)
	gpio_init(PWM_CH2_GPIOx, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, PWM_CH2_GPIO_PINx);
# endif
# if defined (PWM_CH3_GPIOx) && defined (PWM_CH3_GPIO_PINx)
	gpio_init(PWM_CH3_GPIOx, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, PWM_CH3_GPIO_PINx);
# endif
#else
# if defined (PWM_CH0_GPIOx) && defined (PWM_CH0_GPIO_PINx)
	gpio_af_set(PWM_CH0_GPIOx, PWM_GPIO_AFx, PWM_CH0_GPIO_PINx);
	gpio_mode_set(PWM_CH0_GPIOx, GPIO_MODE_AF, GPIO_PUPD_NONE, PWM_CH0_GPIO_PINx);
	gpio_output_options_set(PWM_CH0_GPIOx, GPIO_OTYPE_PP, GPIO_OSPEED, PWM_CH0_GPIO_PINx);
# endif
# if defined (PWM_CH1_GPIOx) && defined (PWM_CH1_GPIO_PINx)
	gpio_af_set(PWM_CH1_GPIOx, PWM_GPIO_AFx, PWM_CH1_GPIO_PINx);
	gpio_mode_set(PWM_CH1_GPIOx, GPIO_MODE_AF, GPIO_PUPD_NONE, PWM_CH1_GPIO_PINx);
	gpio_output_options_set(PWM_CH1_GPIOx, GPIO_OTYPE_PP, GPIO_OSPEED, PWM_CH1_GPIO_PINx);
# endif
# if defined (PWM_CH2_GPIOx) && defined (PWM_CH2_GPIO_PINx)
	gpio_af_set(PWM_CH2_GPIOx, PWM_GPIO_AFx, PWM_CH2_GPIO_PINx);
	gpio_mode_set(PWM_CH2_GPIOx, GPIO_MODE_AF, GPIO_PUPD_NONE, PWM_CH2_GPIO_PINx);
	gpio_output_options_set(PWM_CH2_GPIOx, GPIO_OTYPE_PP, GPIO_OSPEED, PWM_CH2_GPIO_PINx);
# endif
# if defined (PWM_CH3_GPIOx) && defined (PWM_CH3_GPIO_PINx)
	gpio_af_set(PWM_CH3_GPIOx, PWM_GPIO_AFx, PWM_CH3_GPIO_PINx);
	gpio_mode_set(PWM_CH3_GPIOx, GPIO_MODE_AF, GPIO_PUPD_NONE, PWM_CH3_GPIO_PINx);
	gpio_output_options_set(PWM_CH3_GPIOx, GPIO_OTYPE_PP, GPIO_OSPEED, PWM_CH3_GPIO_PINx);
# endif
#endif
}

static void timer_config() {
#if defined (PWM_RCU_TIMERx) && defined (PWM_TIMERx)
	rcu_periph_clock_enable(PWM_RCU_TIMERx);

	timer_deinit(PWM_TIMERx);

	timer_parameter_struct timer_initpara;
	timer_initpara.prescaler         = TIMER_PSC_1MHZ;
	timer_initpara.alignedmode       = TIMER_COUNTER_EDGE;
	timer_initpara.counterdirection  = TIMER_COUNTER_UP;
	timer_initpara.period            = pwm::TIMER_PERIOD;
	timer_initpara.clockdivision     = TIMER_CKDIV_DIV1;
	timer_initpara.repetitioncounter = 0;
	timer_init(PWM_TIMERx, &timer_initpara);

# if defined (PWM_CH0_RCU_GPIOx) || defined (PWM_CH1_RCU_GPIOx) || defined (PWM_CH2_RCU_GPIOx) ||  defined (PWM_CH3_RCU_GPIOx)
	timer_oc_parameter_struct timer_ocintpara;
	timer_ocintpara.ocpolarity   = TIMER_OC_POLARITY_HIGH;
	timer_ocintpara.outputstate  = TIMER_CCX_ENABLE;
	timer_ocintpara.ocnpolarity  = TIMER_OCN_POLARITY_HIGH;
	timer_ocintpara.outputnstate = TIMER_CCXN_DISABLE;
	timer_ocintpara.ocidlestate  = TIMER_OC_IDLE_STATE_LOW;
	timer_ocintpara.ocnidlestate = TIMER_OCN_IDLE_STATE_LOW;
# endif
# if defined (PWM_CH0_RCU_GPIOx)
	timer_channel_output_config(PWM_TIMERx, TIMER_CH_0, &timer_ocintpara);
	timer_channel_output_pulse_value_config(PWM_TIMERx, TIMER_CH_0, 1999);
	timer_channel_output_mode_config(PWM_TIMERx, TIMER_CH_0, TIMER_OC_MODE_PWM0);
	timer_channel_output_shadow_config(PWM_TIMERx, TIMER_CH_0, TIMER_OC_SHADOW_DISABLE);
# endif
# if defined (PWM_CH1_RCU_GPIOx)
	timer_channel_output_config(PWM_TIMERx, TIMER_CH_1, &timer_ocintpara);
	timer_channel_output_pulse_value_config(PWM_TIMERx, TIMER_CH_1, 3999);
	timer_channel_output_mode_config(PWM_TIMERx, TIMER_CH_1, TIMER_OC_MODE_PWM0);
	timer_channel_output_shadow_config(PWM_TIMERx, TIMER_CH_1, TIMER_OC_SHADOW_DISABLE);
# endif
# if defined (PWM_CH2_RCU_GPIOx)
	timer_channel_output_config(PWM_TIMERx, TIMER_CH_2, &timer_ocintpara);
	timer_channel_output_pulse_value_config(PWM_TIMERx, TIMER_CH_2, 7999);
	timer_channel_output_mode_config(PWM_TIMERx, TIMER_CH_2, TIMER_OC_MODE_PWM0);
	timer_channel_output_shadow_config(PWM_TIMERx, TIMER_CH_2, TIMER_OC_SHADOW_DISABLE);
# endif
# if defined (PWM_CH3_RCU_GPIOx)
	timer_channel_output_config(PWM_TIMERx, TIMER_CH_3, &timer_ocintpara);
	timer_channel_output_pulse_value_config(PWM_TIMERx, TIMER_CH_3, 11999);
	timer_channel_output_mode_config(PWM_TIMERx, TIMER_CH_3, TIMER_OC_MODE_PWM0);
	timer_channel_output_shadow_config(PWM_TIMERx, TIMER_CH_3, TIMER_OC_SHADOW_DISABLE);
# endif

	timer_auto_reload_shadow_enable(PWM_TIMERx);
	timer_enable(PWM_TIMERx);
#endif
}

void gd32_pwm_begin() {
	DEBUG_ENTRY

	dump();

	gpio_config();
	timer_config();

#if defined (PWM_CH0_RCU_GPIOx)
	gd32_pwm_set_duty_cycle(pwm::Channel::PWM_CHANNEL_0, pwm::DEFAUL_CHANNEL_0_DUTYCYCLE);
#endif
#if defined (PWM_CH1_RCU_GPIOx)
	gd32_pwm_set_duty_cycle(pwm::Channel::PWM_CHANNEL_1, pwm::DEFAUL_CHANNEL_1_DUTYCYCLE);
#endif
#if defined (PWM_CH2_RCU_GPIOx)
	gd32_pwm_set_duty_cycle(pwm::Channel::PWM_CHANNEL_2, pwm::DEFAUL_CHANNEL_2_DUTYCYCLE);
#endif
#if defined (PWM_CH3_RCU_GPIOx)
	gd32_pwm_set_duty_cycle(pwm::Channel::PWM_CHANNEL_3, pwm::DEFAUL_CHANNEL_3_DUTYCYCLE);
#endif

	DEBUG_EXIT
}

void gd32_pwm_set_duty_cycle(const pwm::Channel channel, const uint32_t nDutyCycle) {
	DEBUG_ENTRY

	const uint32_t nPulse = (nDutyCycle > 100 ? 100 : nDutyCycle) * (pwm::TIMER_PERIOD / 100U);

	DEBUG_PRINTF("Channel=%u, nDutyCycle=%u, nPulse=%u", static_cast<unsigned>(channel), nDutyCycle, static_cast<unsigned>(nPulse));

	switch (channel) {
#if defined (PWM_CH0_RCU_GPIOx)
	case pwm::Channel::PWM_CHANNEL_0:
		timer_channel_output_pulse_value_config(PWM_TIMERx, TIMER_CH_0, nPulse);
		break;
#endif
#if defined (PWM_CH1_RCU_GPIOx)
	case pwm::Channel::PWM_CHANNEL_1:
		timer_channel_output_pulse_value_config(PWM_TIMERx, TIMER_CH_1, nPulse);
		break;
#endif
#if defined (PWM_CH2_RCU_GPIOx)
	case pwm::Channel::PWM_CHANNEL_2:
		timer_channel_output_pulse_value_config(PWM_TIMERx, TIMER_CH_2, nPulse);
		break;
#endif
#if defined (PWM_CH3_RCU_GPIOx)
	case pwm::Channel::PWM_CHANNEL_3:
		timer_channel_output_pulse_value_config(PWM_TIMERx, TIMER_CH_3, nPulse);
		break;
#endif
	default:
		break;
	}

	DEBUG_EXIT
}
#endif
