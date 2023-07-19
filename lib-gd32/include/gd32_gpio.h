/**
 * @file gd32_gpio.h
 *
 */
/* Copyright (C) 2021-2023 by Arjan van Vught mailto:info@gd32-dmx.org
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

#ifndef GD32_GPIO_H_
#define GD32_GPIO_H_

#include <stdint.h>

#define GD32_PORT_TO_GPIO(p,n)	((p * 16) + n)
#define GD32_GPIO_TO_PORT(g)	(uint8_t)(g / 16)
#define GD32_GPIO_TO_NUMBER(g)	(uint8_t)(g - (16 * GD32_GPIO_TO_PORT(g)))

typedef enum T_GD32_Port {
	GD32_GPIO_PORTA = 0,
	GD32_GPIO_PORTB,
	GD32_GPIO_PORTC,
	GD32_GPIO_PORTD,
	GD32_GPIO_PORTE,
	GD32_GPIO_PORTF,
	GD32_GPIO_PORTG,
} GD32_Port_TypeDef;

#include "gd32.h"

#if defined (GD32F10X) || defined (GD32F20X) || defined (GD32F30X)
# define GPIO_FSEL_OUTPUT		GPIO_MODE_OUT_PP
# define GPIO_FSEL_INPUT		GPIO_MODE_IPU
# define GPIO_PULL_UP			GPIO_MODE_IPU
# define GPIO_PULL_DOWN			GPIO_MODE_IPD
# define GPIO_PULL_DISABLE		GPIO_MODE_IN_FLOATING
# define GPIO_INT_CFG_NEG_EDGE	EXTI_TRIG_FALLING
# define GPIO_INT_CFG_BOTH		EXTI_TRIG_BOTH
#elif defined (GD32F407) || defined (GD32F450)
# define GPIO_FSEL_OUTPUT		GPIO_MODE_OUTPUT
# define GPIO_FSEL_INPUT		GPIO_MODE_INPUT
# define GPIO_PULL_UP			GPIO_PUPD_PULLUP
# define GPIO_PULL_DOWN			GPIO_PUPD_PULLDOWN
# define GPIO_PULL_DISABLE		GPIO_PUPD_NONE
#endif

inline static void gpio_fsel(const uint32_t gpio_periph, const uint32_t pin, const uint32_t fsel) {
	switch (gpio_periph) {
		case GPIOA:
			rcu_periph_clock_enable(RCU_GPIOA);
			break;
		case GPIOB:
			rcu_periph_clock_enable(RCU_GPIOB);
			break;
		case GPIOC:
			rcu_periph_clock_enable(RCU_GPIOC);
			break;
		case GPIOD:
			rcu_periph_clock_enable(RCU_GPIOD);
			break;
		case GPIOE:
			rcu_periph_clock_enable(RCU_GPIOE);
			break;
		case GPIOF:
			rcu_periph_clock_enable(RCU_GPIOF);
			break;
		default:
			break;
	}

#if defined  (GD32F10X) || defined (GD32F20X) || defined (GD32F30X)
	if (gpio_periph == GPIOA) {
		if ((pin == GPIO_PIN_13) || (pin == GPIO_PIN_14)) {
			rcu_periph_clock_enable(RCU_AF);
			gpio_pin_remap_config(GPIO_SWJ_DISABLE_REMAP, ENABLE);
		}
	}

	gpio_init(gpio_periph, fsel, GPIO_OSPEED_50MHZ, pin);
#elif  defined (GD32F4XX)
	if (fsel == GPIO_FSEL_OUTPUT) {
		 gpio_mode_set(gpio_periph, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, pin);
		 gpio_output_options_set(gpio_periph, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, pin);
	} else {
		 gpio_mode_set(gpio_periph, GPIO_MODE_INPUT, GPIO_PUPD_PULLUP, pin);
	}
#else
# error MCU not defined
#endif
}

inline static void gd32_gpio_fsel(const uint32_t gpio, const uint32_t fsel) {
	const uint32_t gpio_periph = GPIOA + (GD32_GPIO_TO_PORT(gpio) * 0x400);
	const uint32_t pin = BIT(GD32_GPIO_TO_NUMBER(gpio));

	gpio_fsel(gpio_periph, pin, fsel);
}

inline static void gd32_gpio_pud(const uint32_t gpio, const uint32_t pud) {
	const uint32_t gpio_periph = GPIOA + (GD32_GPIO_TO_PORT(gpio) * 0x400);
	const uint32_t pin = BIT(GD32_GPIO_TO_NUMBER(gpio));

#if defined  (GD32F10X) || defined (GD32F20X) || defined (GD32F30X)
	gpio_init(gpio_periph, pud, GPIO_OSPEED_50MHZ, pin);
#elif  defined (GD32F4XX)
	if (pud == GPIO_PULL_UP) {
		gpio_mode_set(gpio_periph, GPIO_MODE_INPUT, GPIO_PUPD_PULLUP, pin);
	} else if (pud == GPIO_PULL_DOWN) {
		gpio_mode_set(gpio_periph, GPIO_MODE_INPUT, GPIO_PUPD_PULLDOWN, pin);
	} else {
		gpio_mode_set(gpio_periph, GPIO_MODE_INPUT, GPIO_PUPD_NONE, pin);
	}
#endif
}

inline static void gd32_gpio_int_cfg(const uint32_t gpio, const uint32_t trig_type) {
	const uint32_t linex = BIT(GD32_GPIO_TO_NUMBER(gpio));

    switch(trig_type){
    case EXTI_TRIG_RISING:
        EXTI_RTEN |= (uint32_t)linex;
        EXTI_FTEN &= ~(uint32_t)linex;
        break;
    case EXTI_TRIG_FALLING:
        EXTI_RTEN &= ~(uint32_t)linex;
        EXTI_FTEN |= (uint32_t)linex;
        break;
    case EXTI_TRIG_BOTH:
        EXTI_RTEN |= (uint32_t)linex;
        EXTI_FTEN |= (uint32_t)linex;
        break;
    default:
        break;
    }

	const uint8_t output_port = GD32_GPIO_TO_PORT(gpio);
	const uint8_t output_pin = GD32_GPIO_TO_NUMBER(gpio);

#if defined  (GD32F10X) || defined (GD32F20X) || defined (GD32F30X)
    gpio_exti_source_select(output_port, output_pin);
#elif defined (GD32F4XX)
    syscfg_exti_line_config(output_port, output_pin);
#endif
}

inline static void gd32_gpio_clr(const uint32_t gpio) {
	const uint32_t gpio_periph = GPIOA + (GD32_GPIO_TO_PORT(gpio) * 0x400);
	const uint32_t pin = BIT(GD32_GPIO_TO_NUMBER(gpio));

	GPIO_BC(gpio_periph) = (uint32_t) pin;
}

inline static void gd32_gpio_set(const uint32_t gpio) {
	const uint32_t gpio_periph = GPIOA + (GD32_GPIO_TO_PORT(gpio) * 0x400);
	const uint32_t pin = BIT(GD32_GPIO_TO_NUMBER(gpio));

	GPIO_BOP(gpio_periph) = (uint32_t) pin;
}

inline static void gd32_gpio_write(const uint32_t gpio, const uint32_t level) {
	if (level == 0) {
		gd32_gpio_clr(gpio);
	} else {
		gd32_gpio_set(gpio);
	}
}

inline static uint32_t gd32_gpio_lev(const uint32_t gpio) {
	const uint32_t gpio_periph = GPIOA + (GD32_GPIO_TO_PORT(gpio) * 0x400);
	const uint32_t pin = BIT(GD32_GPIO_TO_NUMBER(gpio));
	return (uint32_t) ((uint32_t) 0 != (GPIO_ISTAT(gpio_periph) & pin));
}

#endif /* GD32_GPIO_H_ */
