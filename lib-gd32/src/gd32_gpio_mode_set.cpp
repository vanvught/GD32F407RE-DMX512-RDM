#if defined (GD32F4XX) || defined (GD32H7XX)
/**
 * @file gd32_gpio_mode_set.cpp
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
#include <cassert>

#include "gd32.h"

void gd32_gpio_mode_set_output(const uint32_t gpio_periph, const uint32_t pin) {
	assert(pin != 0);
	assert(__builtin_popcount(static_cast<int>(pin)) == 1);

	auto ctl = GPIO_CTL(gpio_periph);
	auto pupd = GPIO_PUD(gpio_periph);

	const auto i = 31 - __builtin_clz(pin);

	/* clear the specified pin mode bits */
	ctl &= ~GPIO_MODE_MASK(i);
	/* set the specified pin mode bits */
	ctl |= GPIO_MODE_SET(i, GPIO_MODE_OUTPUT);
	/* clear the specified pin pupd bits */
	pupd &= ~GPIO_PUPD_MASK(i);
	/* set the specified pin pupd bits */
	pupd |= GPIO_PUPD_SET(i, GPIO_PUPD_NONE);

	GPIO_CTL(gpio_periph) = ctl;
	GPIO_PUD(gpio_periph) = pupd;
}

void gd32_gpio_mode_set_af(const uint32_t gpio_periph, const uint32_t pin) {
	assert(pin != 0);
	assert(__builtin_popcount(static_cast<int>(pin)) == 1);

	auto ctl = GPIO_CTL(gpio_periph);
	auto pupd = GPIO_PUD(gpio_periph);

	const auto i = 31 - __builtin_clz(pin);

	/* clear the specified pin mode bits */
	ctl &= ~GPIO_MODE_MASK(i);
	/* set the specified pin mode bits */
	ctl |= GPIO_MODE_SET(i, GPIO_MODE_AF);
	/* clear the specified pin pupd bits */
	pupd &= ~GPIO_PUPD_MASK(i);
	/* set the specified pin pupd bits */
	pupd |= GPIO_PUPD_SET(i, GPIO_PUPD_PULLUP);

	GPIO_CTL(gpio_periph) = ctl;
	GPIO_PUD(gpio_periph) = pupd;
}
#endif
