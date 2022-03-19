/**
 * @file gd32_uart.cpp
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

#include "gd32_uart.h"
#include "gd32.h"

static void _rcu_periph_clock_enable(const uint32_t usart_periph) {
#ifndef NDEBUG
	bool isSet = false;
#endif
	switch (usart_periph) {
	case USART0:
		rcu_periph_clock_enable(USART0_RCU_CLK);
		rcu_periph_clock_enable(USART0_GPIO_CLK);
#ifndef NDEBUG
		isSet = true;
#endif
		break;
	case USART1:
		rcu_periph_clock_enable(USART1_RCU_CLK);
		rcu_periph_clock_enable(USART1_GPIO_CLK);
#ifndef NDEBUG
		isSet = true;
#endif
		break;
	case USART2:
		rcu_periph_clock_enable(USART2_RCU_CLK);
		rcu_periph_clock_enable(USART2_GPIO_CLK);
#ifndef NDEBUG
		isSet = true;
#endif
		break;
	case UART3:
		rcu_periph_clock_enable(UART3_RCU_CLK);
		rcu_periph_clock_enable(UART3_GPIO_CLK);
#ifndef NDEBUG
		isSet = true;
#endif
		break;
	case UART4:
		rcu_periph_clock_enable(UART4_RCU_CLK);
		rcu_periph_clock_enable(UART4_GPIO_TX_CLK);
		rcu_periph_clock_enable(UART4_GPIO_RX_CLK);
#ifndef NDEBUG
		isSet = true;
#endif
		break;
#if defined (USART5)
	case USART5:
		rcu_periph_clock_enable(USART5_RCU_CLK);
		rcu_periph_clock_enable(USART5_GPIO_CLK);
# ifndef NDEBUG
		isSet = true;
# endif
		break;
#endif
#if defined (UART6)
	case UART6:
		rcu_periph_clock_enable(UART6_RCU_CLK);
		rcu_periph_clock_enable(UART6_GPIO_CLK);
# ifndef NDEBUG
		isSet = true;
# endif
		break;
#endif
#if defined (UART7)
	case UART7:
		rcu_periph_clock_enable(UART7_RCU_CLK);
		rcu_periph_clock_enable(UART7_GPIO_CLK);
# ifndef NDEBUG
		isSet = true;
# endif
		break;
#endif
	default:
		assert(0);
		__builtin_unreachable();
		break;
	}

	assert(isSet);
}

#if !defined (GD32F4XX)
static void _gpio_init(const uint32_t usart_periph) {
	rcu_periph_clock_enable(RCU_AF);

	switch (usart_periph) {
	case USART0:
		gpio_init(USART0_GPIO_PORT, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, USART0_TX_PIN);
		gpio_init(USART0_GPIO_PORT, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_50MHZ, USART0_RX_PIN);
#if defined (USART0_REMAP)
		gpio_pin_remap_config(GPIO_USART0_REMAP, ENABLE);
#endif
		break;
	case USART1:
		gpio_init(USART1_GPIO_PORT, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, USART1_TX_PIN);
		gpio_init(USART1_GPIO_PORT, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_50MHZ, USART1_RX_PIN);
#if defined (USART1_REMAP)
		gpio_pin_remap_config(GPIO_USART1_REMAP, ENABLE);
#endif
		break;
	case USART2:
		gpio_init(USART2_GPIO_PORT, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, USART2_TX_PIN);
		gpio_init(USART2_GPIO_PORT, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_50MHZ, USART2_RX_PIN);
#if defined (USART2_FULL_REMAP)
		gpio_pin_remap_config(GPIO_USART2_FULL_REMAP, ENABLE);
#elif defined (USART2_PARTIAL_REMAP)
		gpio_pin_remap_config(GPIO_USART2_PARTIAL_REMAP, ENABLE);
#endif
		break;
	case UART3:
		gpio_init(UART3_GPIO_PORT, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, UART3_TX_PIN);
		gpio_init(UART3_GPIO_PORT, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_50MHZ, UART3_RX_PIN);
#if defined (UART3_REMAP)
		gpio_pin_remap_config(GPIO_PCF5_UART3_REMAP, ENABLE);
#endif
		break;
	case UART4:
		gpio_init(UART4_GPIO_TX_PORT, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, UART4_TX_PIN);
		gpio_init(UART4_GPIO_RX_PORT, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_50MHZ, UART4_RX_PIN);
		break;
#if defined (USART5)
	case USART5:
		gpio_init(USART5_GPIO_PORT, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, USART5_TX_PIN);
		gpio_init(USART5_GPIO_PORT, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_50MHZ, USART5_RX_PIN);
# if defined (USART5_REMAP)
		gpio_pin_remap_config(GPIO_PCF5_USART5_TX_PG14_REMAP, ENABLE);
		gpio_pin_remap_config(GPIO_PCF5_USART5_RX_PG9_REMAP, ENABLE);
# endif
		break;
#endif
#if defined (UART6)
	case UART6:
		gpio_init(UART6_GPIO_PORT, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, UART6_TX_PIN);
		gpio_init(UART6_GPIO_PORT, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_50MHZ, UART6_RX_PIN);
# if defined (UART6_REMAP)
		gpio_pin_remap_config(GPIO_PCF5_UART6_REMAP, ENABLE);
# endif
		break;
#endif
#if defined (UART7)
	case UART7:
		gpio_init(UART7_GPIO_PORT, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, UART7_TX_PIN);
		gpio_init(UART7_GPIO_PORT, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_50MHZ, UART7_RX_PIN);
		break;
#endif
	default:
		assert(0);
		__builtin_unreachable();
		break;
	}
}
#else
void _gpio_init(const uint32_t usart_periph) {
	switch (usart_periph) {
	case USART0:
		gpio_af_set(USART0_GPIO_PORT, GPIO_AF_7, USART0_TX_PIN);
		gpio_mode_set(USART0_GPIO_PORT, GPIO_MODE_AF, GPIO_PUPD_PULLUP, USART0_TX_PIN);
		gpio_output_options_set(USART0_GPIO_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, USART0_TX_PIN);
		gpio_af_set(USART0_GPIO_PORT, GPIO_AF_7, USART0_RX_PIN);
		gpio_mode_set(USART0_GPIO_PORT, GPIO_MODE_AF, GPIO_PUPD_PULLUP, USART0_RX_PIN);
		gpio_output_options_set(USART0_GPIO_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, USART0_RX_PIN);
		break;
	case USART1:
		gpio_af_set(USART1_GPIO_PORT, GPIO_AF_7, USART1_TX_PIN);
		gpio_mode_set(USART1_GPIO_PORT, GPIO_MODE_AF, GPIO_PUPD_PULLUP, USART1_TX_PIN);
		gpio_output_options_set(USART1_GPIO_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, USART1_TX_PIN);
		gpio_af_set(USART1_GPIO_PORT, GPIO_AF_7, USART1_RX_PIN);
		gpio_mode_set(USART1_GPIO_PORT, GPIO_MODE_AF, GPIO_PUPD_PULLUP, USART1_RX_PIN);
		gpio_output_options_set(USART1_GPIO_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, USART1_RX_PIN);
		break;
	case USART2:
		gpio_af_set(USART2_GPIO_PORT, GPIO_AF_7, USART2_TX_PIN);
		gpio_mode_set(USART2_GPIO_PORT, GPIO_MODE_AF, GPIO_PUPD_PULLUP, USART2_TX_PIN);
		gpio_output_options_set(USART2_GPIO_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, USART2_TX_PIN);
		gpio_af_set(USART2_GPIO_PORT, GPIO_AF_7, USART2_RX_PIN);
		gpio_mode_set(USART2_GPIO_PORT, GPIO_MODE_AF, GPIO_PUPD_PULLUP, USART2_RX_PIN);
		gpio_output_options_set(USART2_GPIO_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, USART2_RX_PIN);
		break;
	case UART3:
		gpio_af_set(UART3_GPIO_PORT, GPIO_AF_8, UART3_TX_PIN);
		gpio_mode_set(UART3_GPIO_PORT, GPIO_MODE_AF, GPIO_PUPD_PULLUP, UART3_TX_PIN);
		gpio_output_options_set(UART3_GPIO_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, UART3_TX_PIN);
		gpio_af_set(UART3_GPIO_PORT, GPIO_AF_8, UART3_RX_PIN);
		gpio_mode_set(UART3_GPIO_PORT, GPIO_MODE_AF, GPIO_PUPD_PULLUP, UART3_RX_PIN);
		gpio_output_options_set(UART3_GPIO_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, UART3_RX_PIN);
		break;
	case UART4:
		gpio_af_set(UART4_GPIO_TX_PORT, GPIO_AF_8, UART4_TX_PIN);
		gpio_mode_set(UART4_GPIO_TX_PORT, GPIO_MODE_AF, GPIO_PUPD_PULLUP, UART4_TX_PIN);
		gpio_output_options_set(UART4_GPIO_TX_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, UART4_TX_PIN);
		gpio_af_set(UART4_GPIO_RX_PORT, GPIO_AF_8, UART4_RX_PIN);
		gpio_mode_set(UART4_GPIO_RX_PORT, GPIO_MODE_AF, GPIO_PUPD_PULLUP, UART4_RX_PIN);
		gpio_output_options_set(UART4_GPIO_RX_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, UART4_RX_PIN);
		break;
#if defined (USART5)
	case USART5:
		gpio_af_set(USART5_GPIO_PORT, GPIO_AF_8, USART5_TX_PIN);
		gpio_mode_set(USART5_GPIO_PORT, GPIO_MODE_AF, GPIO_PUPD_PULLUP, USART5_TX_PIN);
		gpio_output_options_set(USART5_GPIO_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, USART5_TX_PIN);
		gpio_af_set(USART5_GPIO_PORT, GPIO_AF_8, USART5_RX_PIN);
		gpio_mode_set(USART5_GPIO_PORT, GPIO_MODE_AF, GPIO_PUPD_PULLUP, USART5_RX_PIN);
		gpio_output_options_set(USART5_GPIO_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, USART5_RX_PIN);

		break;
#endif
#if defined (UART6)
	case UART6:
		gpio_af_set(UART6_GPIO_PORT, GPIO_AF_8, UART6_TX_PIN);
		gpio_mode_set(UART6_GPIO_PORT, GPIO_MODE_AF, GPIO_PUPD_PULLUP, UART6_TX_PIN);
		gpio_output_options_set(UART6_GPIO_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, UART6_TX_PIN);
		gpio_af_set(UART6_GPIO_PORT, GPIO_AF_8, UART6_RX_PIN);
		gpio_mode_set(UART6_GPIO_PORT, GPIO_MODE_AF, GPIO_PUPD_PULLUP, UART6_RX_PIN);
		gpio_output_options_set(UART6_GPIO_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, UART6_RX_PIN);
		break;
#endif
#if defined (UART7)
	case UART7:
		gpio_af_set(UART7_GPIO_PORT, GPIO_AF_8, UART7_TX_PIN);
		gpio_mode_set(UART7_GPIO_PORT, GPIO_MODE_AF, GPIO_PUPD_PULLUP, UART7_TX_PIN);
		gpio_output_options_set(UART7_GPIO_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, UART7_TX_PIN);
		gpio_af_set(UART7_GPIO_PORT, GPIO_AF_8, UART7_RX_PIN);
		gpio_mode_set(UART7_GPIO_PORT, GPIO_MODE_AF, GPIO_PUPD_PULLUP, UART7_RX_PIN);
		gpio_output_options_set(UART7_GPIO_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, UART7_RX_PIN);
		break;
#endif
	default:
		assert(0);
		__builtin_unreachable();
		break;
	}
}
#endif

void gd32_uart_begin(const uint32_t usart_periph, uint32_t baudrate, uint32_t bits, uint32_t parity, uint32_t stop_bits) {
	_rcu_periph_clock_enable(usart_periph);
	_gpio_init(usart_periph);

	usart_deinit(usart_periph);

	USART_CTL0(usart_periph) &= ~(USART_CTL0_UEN);

	usart_baudrate_set(usart_periph, baudrate);
	usart_word_length_set(usart_periph, bits == GD32_UART_BITS_9 ? USART_WL_9BIT : USART_WL_8BIT);
	usart_stop_bit_set(usart_periph, stop_bits == GD32_UART_STOP_2BITS ? USART_STB_2BIT : USART_STB_1BIT);

	switch (parity) {
		case GD32_UART_PARITY_EVEN:
			usart_parity_config(usart_periph, USART_PM_EVEN);
			break;
		case GD32_UART_PARITY_ODD:
			usart_parity_config(usart_periph, USART_PM_ODD);
			break;
		default:
			usart_parity_config(usart_periph, USART_PM_NONE);
			break;
	}

	usart_hardware_flow_rts_config(usart_periph, USART_RTS_DISABLE);
	usart_hardware_flow_cts_config(usart_periph, USART_CTS_DISABLE);

	usart_receive_config(usart_periph, USART_RECEIVE_ENABLE);
	usart_transmit_config(usart_periph, USART_TRANSMIT_ENABLE);

	USART_CTL0(usart_periph) |= USART_CTL0_UEN;
}

void gd32_uart_set_baudrate(const uint32_t usart_periph, uint32_t baudrate) {
	assert(baudrate != 0);

	USART_CTL0(usart_periph) &= ~(USART_CTL0_UEN);

	usart_baudrate_set(usart_periph, baudrate);

	USART_CTL0(usart_periph) |= USART_CTL0_UEN;
}

void gd32_uart_transmit(const uint32_t usart_periph, const uint8_t *data, uint32_t length) {
	assert(data != nullptr);

	while (length-- != 0) {
		while (RESET == usart_flag_get(usart_periph, USART_FLAG_TBE))
			;
		USART_DATA(usart_periph) = ((uint16_t) USART_DATA_DATA & *data++);
	}

}

void gd32_uart_transmit_string(const uint32_t usart_periph, const char *data) {
	assert(data != nullptr);

	while (*data != '\0') {
		while (RESET == usart_flag_get(USART0, USART_FLAG_TBE))
			;
		USART_DATA(usart_periph) = ((uint16_t) USART_DATA_DATA & *data++);
	}
}
