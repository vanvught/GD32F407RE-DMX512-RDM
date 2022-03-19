/**
 * @file display_timeout.h
 *
 */
/* Copyright (C) 2022 by Arjan van Vught mailto:info@gd32-dmx.org
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

#ifndef DISPLAY_TIMEOUT_H_
#define DISPLAY_TIMEOUT_H_

#include "gd32.h"

namespace display {
namespace timeout {

void gpio_init() {
    rcu_periph_clock_enable(KEY2_RCU_GPIOx);
#if !defined (GD32F4XX)
    ::gpio_init(KEY2_GPIOx, GPIO_MODE_IPU, GPIO_OSPEED_50MHZ, KEY2_PINx);
#else
    gpio_mode_set(KEY2_GPIOx, GPIO_MODE_INPUT, GPIO_PUPD_PULLUP, KEY2_PINx);
#endif
}

bool gpio_renew() {
	return (GPIO_ISTAT(KEY2_GPIOx) & KEY2_PINx) == 0;
}

}  // namespace timeout
}  // namespace display

#endif /* DISPLAY_TIMEOUT_H_ */
