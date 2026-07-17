/**
 * @file gd32_uart.h
 *
 */
/* Copyright (C) 2021-2026 by Arjan van Vught mailto:info@gd32-dmx.org
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

#include "gd32.h" // IWYU pragma: keep

#if !defined(GD32H7XX)
#define USART_TDATA USART_DATA
#define USART_RDATA USART_DATA
#define USART_TDATA_TDATA USART_DATA_DATA
#define USART_RDATA_TDATA USART_DATA_DATA
#endif

namespace gd32 {
inline constexpr uint32_t kUartBits8 = 8;
inline constexpr uint32_t kUartBits9 = 9;
inline constexpr uint32_t kUartParityNone = 0;
inline constexpr uint32_t kUartParityOdd = 1;
inline constexpr uint32_t kUartParityEven = 2;
inline constexpr uint32_t kUartStop1Bit = 1;
inline constexpr uint32_t kUartStop2Bits = 2;

enum class Uart : uint32_t {
    kUart0 = USART0,
    kUart1 = USART1,
    kUart2 = USART2,
    kUart3 = UART3,
    kUart4 = UART4,
#if defined(USART5)
    kUart5 = USART5,
#endif
#if defined(UART6)
    kUart6 = UART6,
#endif
#if defined(UART7)
    kUart7 = UART7
#endif
};

void UartBegin(uint32_t usart_periph, uint32_t baudrate, uint32_t bits, uint32_t parity, uint32_t stop_bits);
void UartSetBaudrate(uint32_t usart_periph, uint32_t baudrate);

void UartTransmit(uint32_t usart_periph, const uint8_t* data, uint32_t length);
void UartTransmitString(uint32_t usart_periph, const char* data);

inline uint32_t UartGetRxFifoLevel(__attribute__((unused)) uint32_t usart_periph) {
    return 1;
}

inline uint8_t UartGetRxData(uint32_t usart_periph) {
    return static_cast<uint8_t>(GET_BITS(USART_RDATA(usart_periph), 0U, 8U));
}

template <usart_flag_enum kFlag> bool UartFlagGet(uint32_t usart_periph) {
    return (0 != (USART_REG_VAL(usart_periph, kFlag) & BIT(USART_BIT_POS(kFlag))));
}

template <usart_flag_enum kFlag> void UartFlagClear(uint32_t usart_periph) {
#if defined(GD32F10X) || defined(GD32F30X) || defined(GD32F20X)
    USART_REG_VAL(usart_periph, kFlag) = ~BIT(USART_BIT_POS(kFlag));
#elif defined(GD32F4XX)
    USART_REG_VAL(usart_periph, kFlag) &= ~BIT(USART_BIT_POS(kFlag));
#elif defined(GD32H7XX)
    if constexpr (USART_FLAG_AM1 == kFlag) {
        USART_INTC(usart_periph) |= USART_INTC_AMC1;
    } else if constexpr (USART_FLAG_EPERR == kFlag) {
        USART_CHC(usart_periph) &= (uint32_t)(~USART_CHC_EPERR);
    } else if constexpr (USART_FLAG_TFE == kFlag) {
        USART_FCS(usart_periph) |= USART_FCS_TFEC;
    } else {
        USART_INTC(usart_periph) |= BIT(USART_BIT_POS(kFlag));
    }
#else
#error
#endif
}

template <uint32_t kInterrupt> void UartInterruptEnable(uint32_t usart_periph) {
    USART_REG_VAL(usart_periph, kInterrupt) |= BIT(USART_BIT_POS(kInterrupt));
}

template <uint32_t kInterrupt> void UartInterruptDisable(uint32_t usart_periph) {
    USART_REG_VAL(usart_periph, kInterrupt) &= ~BIT(USART_BIT_POS(kInterrupt));
}

template <usart_interrupt_flag_enum kFlag> void UartInterruptFlagClear(uint32_t usart_periph) {
#if defined(GD32F10X) || defined(GD32F30X) || defined(GD32F20X)
    USART_REG_VAL2(usart_periph, kFlag) = ~BIT(USART_BIT_POS2(kFlag));
#elif defined(GD32F4XX)
    USART_REG_VAL2(usart_periph, kFlag) &= ~BIT(USART_BIT_POS2(kFlag));
#elif defined(GD32H7XX)
    if constexpr (USART_INT_FLAG_TFE == kFlag) {
        USART_FCS(usart_periph) |= USART_FCS_TFEC;
    } else if constexpr (USART_INT_FLAG_RFF == kFlag) {
        USART_FCS(usart_periph) &= (~USART_FCS_RFFIF);
    } else {
        USART_INTC(usart_periph) |= BIT(USART_BIT_POS2(kFlag));
    }
#else
#error
#endif
}
} // namespace gd32

#endif // GD32_UART_H_
