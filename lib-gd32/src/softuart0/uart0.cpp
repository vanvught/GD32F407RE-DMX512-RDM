/**
 * @file uart0.c
 *
 */
/* Copyright (C) 2023-2024 by Arjan van Vught mailto:info@gd32-dmx.org
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

#if !defined (CONFIG_REMOTECONFIG_MINIMUM)
# pragma GCC push_options
# pragma GCC optimize ("O2")
#endif

#include <cstdint>
#include <cstdio>

#include "gd32.h"

#include "debug.h"

#if defined (GD32H7XX)
# define TIMERx						TIMER15
# define RCU_TIMERx					RCU_TIMER15
# define TIMERx_IRQHandler			TIMER15_IRQHandler
# define TIMERx_IRQn				TIMER15_IRQn
#else
# define TIMERx						TIMER9
# define RCU_TIMERx					RCU_TIMER9
# define TIMERx_IRQHandler			TIMER0_UP_TIMER9_IRQHandler
# define TIMERx_IRQn				TIMER0_UP_TIMER9_IRQn
#endif

#if defined (GD32H7XX)
# define TIMER_CLOCK_FREQ			(AHB_CLOCK_FREQ)
#elif defined (GD32F4XX)
# define TIMER_CLOCK_FREQ			(APB2_CLOCK_FREQ * 2)
#else
# define TIMER_CLOCK_FREQ			(APB2_CLOCK_FREQ)
#endif

#if !defined (SOFTUART_TX_PINx)
# define SOFTUART_TX_PINx			GPIO_PIN_9
# define SOFTUART_TX_GPIOx			GPIOA
# define SOFTUART_TX_RCU_GPIOx		RCU_GPIOA
#endif

#define BAUD_RATE					(115200U)
#define TIMER_PERIOD				((TIMER_CLOCK_FREQ / BAUD_RATE) - 1U)
#define BUFFER_SIZE					(128U)

typedef enum {
	SOFTUART_IDLE, SOFTUART_START_BIT, SOFTUART_DATA, SOFTUART_STOP_BIT,
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

extern "C" {
void TIMERx_IRQHandler() {
	const auto nIntFlag = TIMER_INTF(TIMERx);

	if ((nIntFlag & TIMER_INT_FLAG_UP) == TIMER_INT_FLAG_UP) {
#if defined (LED3_GPIO_PINx)
		GPIO_BOP(LED3_GPIOx) = LED3_GPIO_PINx;
#endif

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
				timer_disable(TIMERx);
			} else {
				s_state = SOFTUART_START_BIT;
			}
			break;
		default:
			break;
		}

#if defined (LED3_GPIO_PINx)
		GPIO_BC(LED3_GPIOx) = LED3_GPIO_PINx;
#endif
	}

	TIMER_INTF(TIMERx) = static_cast<uint32_t>(~nIntFlag);
}

void uart0_init() {
	s_state = SOFTUART_IDLE;
	s_circular_buffer.head = 0;
	s_circular_buffer.tail = 0;
	s_circular_buffer.full = false;

#if defined (LED3_GPIO_PINx)
	rcu_periph_clock_enable (LED3_RCU_GPIOx);
# if defined (GPIO_INIT)
	gpio_init(LED3_GPIOx, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, LED3_GPIO_PINx);
# else
	gpio_mode_set(LED3_GPIOx, GPIO_MODE_OUTPUT, GPIO_PUPD_PULLDOWN, LED3_GPIO_PINx);
	gpio_output_options_set(LED3_GPIOx, GPIO_OTYPE_PP, GPIO_OSPEED, LED3_GPIO_PINx);
# endif

	GPIO_BC(LED3_GPIOx) = LED3_GPIO_PINx;
#endif

	rcu_periph_clock_enable (SOFTUART_TX_RCU_GPIOx);

#if defined (GPIO_INIT)
	gpio_init(SOFTUART_TX_GPIOx, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, SOFTUART_TX_PINx);
#else
	gpio_mode_set(SOFTUART_TX_GPIOx, GPIO_MODE_OUTPUT, GPIO_PUPD_PULLDOWN, SOFTUART_TX_PINx);
	gpio_output_options_set(SOFTUART_TX_GPIOx, GPIO_OTYPE_PP, GPIO_OSPEED, SOFTUART_TX_PINx);
#endif

	GPIO_BOP(SOFTUART_TX_GPIOx) = SOFTUART_TX_PINx;

	rcu_periph_clock_enable(RCU_TIMERx);

	timer_deinit(TIMERx);

	timer_parameter_struct timer_initpara;
	timer_struct_para_init(&timer_initpara);

	timer_initpara.prescaler = 0;
	timer_initpara.alignedmode = TIMER_COUNTER_EDGE;
	timer_initpara.counterdirection = TIMER_COUNTER_UP;
	timer_initpara.period = TIMER_PERIOD;
	timer_initpara.clockdivision = TIMER_CKDIV_DIV1;
	timer_initpara.repetitioncounter = 0;

	timer_init(TIMERx, &timer_initpara);

	timer_flag_clear(TIMERx, ~0);
	timer_interrupt_flag_clear(TIMERx, ~0);

	timer_interrupt_enable(TIMERx, TIMER_INT_UP);

	NVIC_SetPriority(TIMERx_IRQn, 2);
	NVIC_EnableIRQ(TIMERx_IRQn);
}

static void _putc(const int c) {
	//FIXME deadlock when timer is not running
	while (s_circular_buffer.full)
		;

	s_circular_buffer.buffer[s_circular_buffer.head] = (uint8_t) c;
	s_circular_buffer.head = (s_circular_buffer.head + 1) & (BUFFER_SIZE - 1);
	s_circular_buffer.full = s_circular_buffer.head == s_circular_buffer.tail;

	if (s_state == SOFTUART_IDLE) {
		timer_counter_value_config(TIMERx, 0);
		timer_enable(TIMERx);
		s_state = SOFTUART_START_BIT;
	}
}

void uart0_putc(int c) {
	if (c == '\n') {
		_putc('\r');
	}

	_putc(c);
}

void uart0_puts(const char *s) {
	while (*s != '\0') {
		if (*s == '\n') {
			uart0_putc('\r');
		}
		uart0_putc(*s++);
	}

	do {
		__DMB();
	} while (!is_circular_buffer_empty());
}
}
