/**
 * @file gd32_uart0.c
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

#include <stdint.h>
#include <stdio.h>

#include "gd32.h"
#include "gd32_uart.h"

void uart0_init(void) {
	gd32_uart_begin(USART0, 115200U, GD32_UART_BITS_8, GD32_UART_PARITY_NONE, GD32_UART_STOP_1BIT);
}

void uart0_putc(int c) {
	if (c == '\n') {
		while (RESET == usart_flag_get(USART0, USART_FLAG_TBE))
			;
		USART_DATA(USART0) = ((uint16_t) USART_DATA_DATA & (uint8_t) '\r');
	}

	while (RESET == usart_flag_get(USART0, USART_FLAG_TBE))
		;

	USART_DATA(USART0) = ((uint16_t) USART_DATA_DATA & (uint8_t) c);
}
