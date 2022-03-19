/**
 * @file micros.cpp
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

#include <cstdint>
#include <cassert>

#include "gd32.h"

#if defined (GD32F20X_CL)

/**
 * Timer 9 is Master -> TIMER9_TRGO
 * Timer 8 is Slave -> ITI2
 */

static void _master_init(void) {
	timer_parameter_struct timer_initpara;

	rcu_periph_clock_enable(RCU_TIMER9);

	timer_deinit(TIMER9);
	TIMER_CNT(TIMER9) = 0;

	timer_initpara.prescaler = 119;	///< TIMER9CLK = SystemCoreClock / 120 = 1MHz => us ticker
	timer_initpara.alignedmode = TIMER_COUNTER_EDGE;
	timer_initpara.counterdirection = TIMER_COUNTER_UP;
	timer_initpara.period = (uint32_t)(~0);
	timer_initpara.clockdivision = TIMER_CKDIV_DIV1;

	timer_init(TIMER9, &timer_initpara);

	timer_master_slave_mode_config(TIMER9, TIMER_MASTER_SLAVE_MODE_DISABLE);
	timer_master_output_trigger_source_select(TIMER9, TIMER_TRI_OUT_SRC_UPDATE);

	timer_enable(TIMER9);
}

static void _slave_init(void) {
	timer_parameter_struct timer_initpara;

	rcu_periph_clock_enable(RCU_TIMER8);

	timer_deinit(TIMER8);
	TIMER_CNT(TIMER8) = 0;

	timer_initpara.prescaler = 0;
	timer_initpara.alignedmode = TIMER_COUNTER_EDGE;
	timer_initpara.counterdirection = TIMER_COUNTER_UP;
	timer_initpara.period = (uint32_t)(~0);
	timer_initpara.clockdivision = TIMER_CKDIV_DIV1;

	timer_init(TIMER8, &timer_initpara);

	timer_master_slave_mode_config(TIMER8, TIMER_MASTER_SLAVE_MODE_DISABLE);
	timer_slave_mode_select(TIMER8, TIMER_SLAVE_MODE_EXTERNAL0);
	timer_input_trigger_source_select(TIMER8, TIMER_SMCFG_TRGSEL_ITI2);

	timer_enable(TIMER8);
}

void micros_init(void) {
	_slave_init();
	_master_init();
}
#else
void micros_init(void) {
}
#endif
