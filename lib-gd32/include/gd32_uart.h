/**
 * @file gd32_uart.h
 *
 */
/* Copyright (C) 2021 by Arjan van Vught mailto:info@gd32-dmx.org
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

#include <stdint.h>
#include "gd32.h"

#ifdef __cplusplus
extern "C" {
#endif

extern void gd32_uart_begin(const uint32_t usart_periph, uint32_t baudrate, uint32_t bits, uint32_t parity, uint32_t stop_bits);
extern void gd32_uart_set_baudrate(const uint32_t usart_periph, uint32_t baudrate);
extern void gd32_uart_transmit(const uint32_t usart_periph, const uint8_t *data, uint32_t length);
extern void gd32_uart_transmit_string(const uint32_t uart_base, const char *data);

static inline uint32_t gd32_uart_get_rx_fifo_level(const uint32_t usart_periph) {
	return 1;
}

static inline uint8_t gd32_uart_get_rx_data(const uint32_t uart_base) {
	return (uint8_t)(GET_BITS(USART_DATA(uart_base), 0U, 8U));
}

#ifdef __cplusplus
}
#endif
#endif /* GD32_UART_H_ */
