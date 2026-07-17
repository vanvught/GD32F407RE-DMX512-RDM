/**
 * @file board_dmx3.h
 *
 */
/* Copyright (C) 2023-2026 by Arjan van Vught mailto:info@gd32-dmx.org
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

#ifndef DMX_BOARD_DMX3_H_
#define DMX_BOARD_DMX3_H_

#include <cstdint>

#include "gd32.h" // IWYU pragma: keep
#include "gd32/dmx_port.h"

#define DMX_MAX_PORTS 3

namespace dmx::config {
namespace max {
inline constexpr uint32_t kPorts = DMX_MAX_PORTS;
} // namespace max

#define DMX_USE_USART2
#define DMX_USE_UART4
#define DMX_USE_USART5

inline constexpr port::Info kPort0 = {.uart = gd32::Uart::kUart2, .port = GPIOB, .pin = GPIO_PIN_10, .usage = port::Usage::kTxRx};
inline constexpr port::Info kPort1 = {.uart = gd32::Uart::kUart4, .port = GPIOA, .pin = GPIO_PIN_5, .usage = port::Usage::kTxRx};
inline constexpr port::Info kPort2 = {.uart = gd32::Uart::kUart5, .port = GPIOB, .pin = GPIO_PIN_14, .usage = port::Usage::kTxRx};
} // namespace dmx::config
#endif // DMX_BOARD_DMX3_H_
