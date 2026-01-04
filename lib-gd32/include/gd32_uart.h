/**
 * @file gd32_uart.h
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

#ifndef GD32_UART_H_
#define GD32_UART_H_

#include <cstdint>

#include "gd32.h"

#if !defined(GD32H7XX)
#define USART_TDATA USART_DATA
#define USART_RDATA USART_DATA
#define USART_TDATA_TDATA USART_DATA_DATA
#define USART_RDATA_TDATA USART_DATA_DATA
#endif

namespace gd32
{
inline constexpr uint32_t kUartBits8 = 8;
inline constexpr uint32_t kUartBits9 = 9;
inline constexpr uint32_t kUartParityNone = 0;
inline constexpr uint32_t kUartParityOdd = 1;
inline constexpr uint32_t kUartParityEven = 2;
inline constexpr uint32_t kUartStop1Bit = 1;
inline constexpr uint32_t kUartStop2Bits = 2;
} // namespace gd32

void Gd32UartBegin(uint32_t usart_periph, uint32_t baudrate, uint32_t bits, uint32_t parity, uint32_t stop_bits);
void Gd32UartSetBaudrate(uint32_t usart_periph, uint32_t baudrate);

void Gd32UartTransmit(uint32_t usart_periph, const uint8_t* data, uint32_t length);
void Gd32UartTransmitString(uint32_t usart_periph, const char* data);

inline uint32_t Gd32UartGetRxFifoLevel(__attribute__((unused)) uint32_t usart_periph)
{
    return 1;
}

inline uint8_t Gd32UartGetRxData(uint32_t usart_periph)
{
    return static_cast<uint8_t>(GET_BITS(USART_RDATA(usart_periph), 0U, 8U));
}

template <usart_flag_enum flag> bool Gd32UsartFlagGet(uint32_t usart_periph)
{
    return (0 != (USART_REG_VAL(usart_periph, flag) & BIT(USART_BIT_POS(flag))));
}

template <usart_flag_enum flag> void Gd32UsartFlagClear(uint32_t usart_periph)
{
#if defined(GD32F10X) || defined(GD32F30X) || defined(GD32F20X)
    USART_REG_VAL(usart_periph, flag) = ~BIT(USART_BIT_POS(flag));
#elif defined(GD32F4XX)
    USART_REG_VAL(usart_periph, flag) &= ~BIT(USART_BIT_POS(flag));
#elif defined(GD32H7XX)
    if constexpr (USART_FLAG_AM1 == flag)
    {
        USART_INTC(usart_periph) |= USART_INTC_AMC1;
    }
    else if constexpr (USART_FLAG_EPERR == flag)
    {
        USART_CHC(usart_periph) &= (uint32_t)(~USART_CHC_EPERR);
    }
    else if constexpr (USART_FLAG_TFE == flag)
    {
        USART_FCS(usart_periph) |= USART_FCS_TFEC;
    }
    else
    {
        USART_INTC(usart_periph) |= BIT(USART_BIT_POS(flag));
    }
#else
#error
#endif
}

template <uint32_t interrupt> void Gd32UsartInterruptEnable(uint32_t usart_periph)
{
    USART_REG_VAL(usart_periph, interrupt) |= BIT(USART_BIT_POS(interrupt));
}

template <uint32_t interrupt> void Gd32UsartInterruptDisable(uint32_t usart_periph)
{
    USART_REG_VAL(usart_periph, interrupt) &= ~BIT(USART_BIT_POS(interrupt));
}

template <usart_interrupt_flag_enum flag> void Gd32UsartInterruptFlagClear(uint32_t usart_periph)
{
#if defined(GD32F10X) || defined(GD32F30X) || defined(GD32F20X)
    USART_REG_VAL2(usart_periph, flag) = ~BIT(USART_BIT_POS2(flag));
#elif defined(GD32F4XX)
    USART_REG_VAL2(usart_periph, flag) &= ~BIT(USART_BIT_POS2(flag));
#elif defined(GD32H7XX)
    if constexpr (USART_INT_FLAG_TFE == flag)
    {
        USART_FCS(usart_periph) |= USART_FCS_TFEC;
    }
    else if constexpr (USART_INT_FLAG_RFF == flag)
    {
        USART_FCS(usart_periph) &= (~USART_FCS_RFFIF);
    }
    else
    {
        USART_INTC(usart_periph) |= BIT(USART_BIT_POS2(flag));
    }
#else
#error
#endif
}

#endif // GD32_UART_H_
