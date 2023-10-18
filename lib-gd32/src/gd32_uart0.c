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

#include <stdarg.h>
#include <stdio.h>

extern void uart0_putc(int);

static char s_buffer[128];

void uart0_puts(const char *s) {
	while (*s != '\0') {
		if (*s == '\n') {
			uart0_putc('\r');
		}
		uart0_putc(*s++);
	}

//	uart0_putc('\n'); //TODO Add '\n'
}

int uart0_printf(const char *fmt, ...) {
	va_list arp;

	va_start(arp, fmt);

	int i = vsnprintf(s_buffer, sizeof(s_buffer) - 1, fmt, arp);

	va_end(arp);

	char *s = s_buffer;

	while (*s != '\0') {
		if (*s == '\n') {
			uart0_putc('\r');
		}
		uart0_putc(*s++);
	}

	return i;
}
