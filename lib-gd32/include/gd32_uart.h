/**
 * @file gd32_uart.h
 *
 */
/* Copyright (C) 2021-2024 by Arjan van Vught mailto:info@gd32-dmx.org
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

#ifndef GD32_UART_H_
#define GD32_UART_H_

#if !defined (GD32H7XX)
# define USART_TDATA						USART_DATA
# define USART_RDATA						USART_DATA
# define USART_TDATA_TDATA					USART_DATA_DATA
# define USART_RDATA_TDATA					USART_DATA_DATA
#endif

typedef enum GD32_UART_BITS {
	GD32_UART_BITS_8 = 8,
	GD32_UART_BITS_9 = 9
} gd32_uart_bits_t;

typedef enum GD32_UART_PARITY {
	GD32_UART_PARITY_NONE = 0,
	GD32_UART_PARITY_ODD = 1,
	GD32_UART_PARITY_EVEN = 2
} gd32_uart_parity_t;

typedef enum GD32_UART_STOPBITS {
	GD32_UART_STOP_1BIT = 1,
	GD32_UART_STOP_2BITS = 2
} gd32_uart_stopbits_t;

#include <cstdint>
#include "gd32.h"

void gd32_uart_begin(const uint32_t usart_periph, uint32_t baudrate, uint32_t bits, uint32_t parity, uint32_t stop_bits);
void gd32_uart_set_baudrate(const uint32_t usart_periph, uint32_t baudrate);
void gd32_uart_transmit(const uint32_t usart_periph, const uint8_t *data, uint32_t length);
void gd32_uart_transmit_string(const uint32_t usart_periph, const char *data);

inline uint32_t gd32_uart_get_rx_fifo_level(__attribute__((unused)) const uint32_t usart_periph) {
	return 1;
}

inline uint8_t gd32_uart_get_rx_data(const uint32_t usart_periph) {
	return static_cast<uint8_t>(GET_BITS(USART_RDATA(usart_periph), 0U, 8U));
}

template<usart_flag_enum flag>
bool gd32_usart_flag_get(const uint32_t usart_periph) {
	return (0 != (USART_REG_VAL(usart_periph, flag) & BIT(USART_BIT_POS(flag))));
}

template<usart_flag_enum flag>
void gd32_usart_flag_clear(const uint32_t usart_periph) {
#if defined (GD32F10X) || defined (GD32F30X) || defined (GD32F20X)
    USART_REG_VAL(usart_periph, flag) = ~BIT(USART_BIT_POS(flag));
#elif defined (GD32F4XX)
    USART_REG_VAL(usart_periph, flag) &= ~BIT(USART_BIT_POS(flag));
#elif defined (GD32H7XX)
	if constexpr (USART_FLAG_AM1 == flag) {
		USART_INTC(usart_periph) |= USART_INTC_AMC1;
	} else if constexpr (USART_FLAG_EPERR == flag) {
		USART_CHC(usart_periph) &= (uint32_t) (~USART_CHC_EPERR);
	} else if constexpr (USART_FLAG_TFE == flag) {
		USART_FCS(usart_periph) |= USART_FCS_TFEC;
	} else {
		USART_INTC(usart_periph) |= BIT(USART_BIT_POS(flag));
	}
#else
# error
#endif
}

template<uint32_t interrupt>
void gd32_usart_interrupt_enable(const uint32_t usart_periph) {
	USART_REG_VAL(usart_periph, interrupt) |= BIT(USART_BIT_POS(interrupt));
}

template<uint32_t interrupt>
void gd32_usart_interrupt_disable(const uint32_t usart_periph) {
	USART_REG_VAL(usart_periph, interrupt) &= ~BIT(USART_BIT_POS(interrupt));
}

template<usart_interrupt_flag_enum flag>
void gd32_usart_interrupt_flag_clear(const uint32_t usart_periph) {
#if defined (GD32F10X) || defined (GD32F30X) || defined (GD32F20X)
    USART_REG_VAL2(usart_periph, flag) = ~BIT(USART_BIT_POS2(flag));
#elif defined (GD32F4XX)
    USART_REG_VAL2(usart_periph, flag) &= ~BIT(USART_BIT_POS2(flag));
#elif defined (GD32H7XX)
	if constexpr (USART_INT_FLAG_TFE == flag) {
		USART_FCS(usart_periph) |= USART_FCS_TFEC;
	} else if constexpr (USART_INT_FLAG_RFF == flag) {
		USART_FCS(usart_periph) &= (~USART_FCS_RFFIF);
	} else {
		USART_INTC(usart_periph) |= BIT(USART_BIT_POS2(flag));
	}
#else
# error
#endif
}

#endif /* GD32_UART_H_ */
