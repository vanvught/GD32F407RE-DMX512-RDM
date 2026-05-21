/**
 * @file board_bw_opidmx4.h
 *
 */
/* Copyright (C) 2022-2026 by Arjan van Vught mailto:info@gd32-dmx.org
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

#ifndef DMX_BOARD_BW_OPIDMX4_H_
#define DMX_BOARD_BW_OPIDMX4_H_

#include <cstdint>

#include "gd32.h" // IWYU pragma: keep

#define DMX_MAX_PORTS 4

namespace dmx::config {
namespace max {
inline constexpr uint32_t kPorts = DMX_MAX_PORTS;
} // namespace max

#define DMX_USE_USART0
#define DMX_USE_USART2
#define DMX_USE_UART4
#define DMX_USE_USART5

inline constexpr auto kUsart0Port = 3; // OPi One UART0
inline constexpr auto kUsart2Port = 2; // OPi One UART3
inline constexpr auto kUart4Port = 0;  // OPi One UART1	Pin 38 TX, Pin 40 RX
inline constexpr auto kUsart5Port = 1; // OPi One UART2

inline constexpr auto kDirPort0GpioPort = GPIOA;     // OPi One UART1
inline constexpr auto kDirPort0GpioPin = GPIO_PIN_4; // GPIO_EXT_32

inline constexpr auto kDirPort1GpioPort = GPIOA;      // OPi One UART2
inline constexpr auto kDirPort1GpioPin = GPIO_PIN_11; // GPIO_EXT_22

inline constexpr auto kDirPort2GpioPort = GPIOB;      // OPi One UART3
inline constexpr auto kDirPort2GpioPin = GPIO_PIN_10; // GPIO_EXT_12

inline constexpr auto kDirPort3GpioPort = GPIOA;     // OPi One UART0
inline constexpr auto kDirPort3GpioPin = GPIO_PIN_5; // GPIO_EXT_31
} // namespace dmx::config
#endif // DMX_BOARD_BW_OPIDMX4_H_
