/**
 * @file uart0.cpp
 *
 */
/* Copyright (C) 2021-2025 by Arjan van Vught mailto:info@gd32-dmx.org
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

namespace uart0
{
static char s_buffer[128];

void Init()
{
    Gd32UartBegin(USART0, 115200U, gd32::kUartBits8, gd32::kUartParityNone, gd32::kUartStop1Bit);
}

void Putc(int c)
{
    if (c == '\n')
    {
        while (RESET == usart_flag_get(USART0, USART_FLAG_TBE));
#if defined(GD32H7XX)
        USART_TDATA(USART0) = USART_TDATA_TDATA & (uint32_t)'\r';
#else
        USART_DATA(USART0) = static_cast<uint16_t>(USART_DATA_DATA & static_cast<uint8_t>('\r'));
#endif
    }

    while (RESET == usart_flag_get(USART0, USART_FLAG_TBE));
#if defined(GD32H7XX)
    USART_TDATA(USART0) = USART_TDATA_TDATA & (uint32_t)c;
#else
    USART_DATA(USART0) = static_cast<uint16_t>(USART_DATA_DATA & static_cast<uint8_t>(c));
#endif
}

int Printf(const char* fmt, ...)
{
    va_list arp;

    va_start(arp, fmt);

    int i = vsnprintf(s_buffer, sizeof(s_buffer), fmt, arp);
    s_buffer[sizeof(s_buffer) - 1] = '\0';

    va_end(arp);

    char* s = s_buffer;

    while (*s != '\0')
    {
        if (*s == '\n')
        {
            Putc('\r');
        }

        Putc(*s++);
    }

    return i;
}

void Puts(const char* s)
{
    while (*s != '\0')
    {
        if (*s == '\n')
        {
            Putc('\r');
        }
        Putc(*s++);
    }

    Putc('\n');
}

int Getc()
{
    if (__builtin_expect((!Gd32UsartFlagGet<USART_FLAG_RBNE>(USART0)), 1))
    {
        return EOF;
    }

#if defined(GD32H7XX)
    const auto kC = static_cast<int>(USART_RDATA(USART0));
#else
    const auto kC = static_cast<int>(USART_DATA(USART0));
#endif

#if defined(UART0_ECHO)
    Putc(c);
#endif

    return kC;
}
} // namespace uart0
