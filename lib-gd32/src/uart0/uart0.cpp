/**
 * @file uart0.cpp
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

#include <cstdint>
#include <cstdio>

#include "gd32.h"
#include "gd32_uart.h"

extern "C" {
void uart0_init(void) {
	gd32_uart_begin(USART0, 115200U, GD32_UART_BITS_8, GD32_UART_PARITY_NONE, GD32_UART_STOP_1BIT);
}

void uart0_putc(int c) {
	if (c == '\n') {
		while (RESET == usart_flag_get(USART0, USART_FLAG_TBE))
			;
#if defined (GD32H7XX)
		USART_TDATA(USART0) = USART_TDATA_TDATA & (uint32_t)'\r';
#else
		USART_DATA(USART0) = ((uint16_t) USART_DATA_DATA & (uint8_t) '\r');
#endif
	}

	while (RESET == usart_flag_get(USART0, USART_FLAG_TBE))
		;
#if defined (GD32H7XX)
	USART_TDATA(USART0) = USART_TDATA_TDATA & (uint32_t) c;
#else
	USART_DATA(USART0) = ((uint16_t) USART_DATA_DATA & (uint8_t) c);
#endif
}

void uart0_puts(const char *s) {
	while (*s != '\0') {
		if (*s == '\n') {
			uart0_putc('\r');
		}
		uart0_putc(*s++);
	}
}

int uart0_getc(void) {
	if (__builtin_expect((!gd32_usart_flag_get<USART_FLAG_RBNE>(USART0)), 1)) {
		return EOF;
	}

#if defined (GD32H7XX)
	const auto c = static_cast<int>(USART_RDATA(USART0));
#else
	const auto c = static_cast<int>(USART_DATA(USART0));
#endif

#if defined (UART0_ECHO)
	uart0_putc(c);
#endif

	return c;
}
}
