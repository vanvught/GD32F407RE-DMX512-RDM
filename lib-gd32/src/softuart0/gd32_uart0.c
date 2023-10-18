/**
 * @file gd32_uart0.c
 *
 */
/* Copyright (C) 2023 by Arjan van Vught mailto:info@gd32-dmx.org
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

#if defined (NDEBUG)
# undef NDEBUG
#endif

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>

#include "gd32.h"

#if !defined (SOFTUART_TX_PINx)
# define SOFTUART_TX_PINx		GPIO_PIN_9
# define SOFTUART_TX_GPIOx		GPIOA
# define SOFTUART_TX_RCU_GPIOx	RCU_GPIOA
#endif

#if defined (GD32F4XX)
# define TIMER_CLOCK		(APB2_CLOCK_FREQ * 2)
#else
# define TIMER_CLOCK		(APB2_CLOCK_FREQ)
#endif

#define BAUD_RATE		(115200U)
#define TIMER_PERIOD	((TIMER_CLOCK / BAUD_RATE) - 1U)
#define BUFFER_SIZE		(128U)

typedef enum  {
	SOFTUART_IDLE,
	SOFTUART_START_BIT,
	SOFTUART_DATA,
	SOFTUART_STOP_BIT,
} softuart_state;

struct circular_buffer {
	uint8_t buffer[BUFFER_SIZE];
	uint32_t head;
	uint32_t tail;
	bool full;
};

static volatile softuart_state s_state;
static volatile struct circular_buffer s_circular_buffer;
static volatile uint8_t s_data;
static volatile uint8_t s_shift;

static bool is_circular_buffer_empty() {
	return (!s_circular_buffer.full && (s_circular_buffer.head == s_circular_buffer.tail));
}

void TIMER0_UP_TIMER9_IRQHandler() {
	GPIO_BOP(LED3_GPIOx) = LED3_GPIO_PINx;

	switch (s_state) {
	case SOFTUART_IDLE:
		break;
	case SOFTUART_START_BIT:
		GPIO_BC(SOFTUART_TX_GPIOx) = SOFTUART_TX_PINx;

		s_state = SOFTUART_DATA;
		s_data = s_circular_buffer.buffer[s_circular_buffer.tail];
		s_circular_buffer.tail = (s_circular_buffer.tail + 1) & (BUFFER_SIZE - 1);
		s_circular_buffer.full = false;
		s_shift = 0;
		break;
	case SOFTUART_DATA:
		if (s_data & (1U << s_shift)) {
			GPIO_BOP(SOFTUART_TX_GPIOx) = SOFTUART_TX_PINx;
		} else {
			GPIO_BC(SOFTUART_TX_GPIOx) = SOFTUART_TX_PINx;
		}

		s_shift++;

		if (s_shift == 8) {
			s_state = SOFTUART_STOP_BIT;
		}
		break;
	case SOFTUART_STOP_BIT:
		GPIO_BOP(SOFTUART_TX_GPIOx) = SOFTUART_TX_PINx;

		if (is_circular_buffer_empty()) {
			s_state = SOFTUART_IDLE;
			timer_disable(TIMER9);
		} else {
			s_state = SOFTUART_START_BIT;
		}
		break;
	default:
		break;
	}

	timer_interrupt_flag_clear(TIMER9, TIMER_INT_FLAG_UP);

	GPIO_BC(LED3_GPIOx) = LED3_GPIO_PINx;
}

void uart0_init() {
	rcu_periph_clock_enable (LED3_RCU_GPIOx);
#if !defined (GD32F4XX)
	gpio_init(LED3_GPIOx, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, LED3_GPIO_PINx);
#else
	gpio_mode_set(LED3_GPIOx, GPIO_MODE_OUTPUT, GPIO_PUPD_PULLDOWN, LED3_GPIO_PINx);
	gpio_output_options_set(LED3_GPIOx, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, LED3_GPIO_PINx);
	gpio_af_set(LED3_GPIOx, GPIO_AF_0, LED3_GPIO_PINx);
#endif

	GPIO_BC(LED3_GPIOx) = LED3_GPIO_PINx;

	rcu_periph_clock_enable (SOFTUART_TX_RCU_GPIOx);

#if !defined (GD32F4XX)
	gpio_init(SOFTUART_TX_GPIOx, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, SOFTUART_TX_PINx);
#else
	gpio_mode_set(SOFTUART_TX_GPIOx, GPIO_MODE_OUTPUT, GPIO_PUPD_PULLDOWN, SOFTUART_TX_PINx);
	gpio_output_options_set(SOFTUART_TX_GPIOx, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, SOFTUART_TX_PINx);
	gpio_af_set(SOFTUART_TX_GPIOx, GPIO_AF_0, SOFTUART_TX_PINx);
#endif

	GPIO_BOP(SOFTUART_TX_GPIOx) = SOFTUART_TX_PINx;

	rcu_periph_clock_enable(RCU_TIMER9);

	timer_deinit(TIMER9);

	timer_parameter_struct timer_initpara;
	timer_initpara.prescaler = 0;
	timer_initpara.alignedmode = TIMER_COUNTER_EDGE;
	timer_initpara.counterdirection = TIMER_COUNTER_UP;
	timer_initpara.period = TIMER_PERIOD;
	timer_initpara.clockdivision = TIMER_CKDIV_DIV1;

	timer_init(TIMER9, &timer_initpara);

	timer_flag_clear(TIMER9, ~0);
	timer_interrupt_flag_clear(TIMER9, ~0);

	timer_interrupt_enable(TIMER9, TIMER_INT_UP);

	NVIC_SetPriority(TIMER0_UP_TIMER9_IRQn, 2);
	NVIC_EnableIRQ(TIMER0_UP_TIMER9_IRQn);
}

static void _putc(int c) {
	while (s_circular_buffer.full)
		;

	s_circular_buffer.buffer[s_circular_buffer.head] = (uint8_t) c;
	s_circular_buffer.head = (s_circular_buffer.head + 1) & (BUFFER_SIZE - 1);
	s_circular_buffer.full = s_circular_buffer.head == s_circular_buffer.tail;

	if (s_state == SOFTUART_IDLE) {
		timer_counter_value_config(TIMER9, 0);
		timer_enable(TIMER9);
		s_state = SOFTUART_START_BIT;
	}
}

void uart0_putc(int c) {
	if (c == '\n') {
		_putc('\r');
	}

	_putc(c);
}
