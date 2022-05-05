/**
 * @file gd32_micros.h
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

#ifndef GD32_MICROS_H_
#define GD32_MICROS_H_

#include <cstdint>

#include "gd32.h"

#if defined (GD32F20X_CL)
static inline uint32_t micros() {
	uint32_t msw, lsw;
	do {
		msw = TIMER_CNT(TIMER9);
		lsw = TIMER_CNT(TIMER8);
	} while (msw != TIMER_CNT(TIMER9));
	return (msw << 16) | lsw;
}
#elif defined (GD32F4XX)
static inline uint32_t micros() {
	return TIMER_CNT(TIMER4);
}
#else
static inline uint32_t micros() {
	return DWT->CYCCNT / (MCU_CLOCK_FREQ / 1000000U);
}
#endif

#endif /* GD32_MICROS_H_ */
