/**
 * @file gd32_gpio_macros.h
 *
 */
/* Copyright (C) 2026 by Arjan van Vught mailto:info@gd32-dmx.org
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

#ifndef GD32_GPIO_MACROS_H_
#define GD32_GPIO_MACROS_H_

using GD32_Port_TypeDef = enum T_GD32_Port { 
  GD32_GPIO_PORTA = 0, 
  GD32_GPIO_PORTB, 
  GD32_GPIO_PORTC, 
  GD32_GPIO_PORTD, 
  GD32_GPIO_PORTE, 
  GD32_GPIO_PORTF, 
  GD32_GPIO_PORTG, 
  GD32_GPIO_PORTH, 
  GD32_GPIO_PORTI, 
  GD32_GPIO_PORTJ, 
  GD32_GPIO_PORTK 
};

#ifdef __cplusplus
#include <cstdint>
namespace gd32 {
constexpr uint32_t kGPioPins = 16;
constexpr uint32_t PortToGpio(uint32_t port, uint32_t pin) {
    return (port * kGPioPins) + pin;
}

constexpr uint8_t GpioToPort(uint32_t gpio) {
    return static_cast<uint8_t>(gpio / kGPioPins);
}

constexpr uint8_t GpioToNumber(uint32_t gpio) {
    return static_cast<uint8_t>(gpio % kGPioPins);
}
} // namespace gd32

#define GD32_PORT_TO_GPIO(p, n) gd32::PortToGpio((p), (n))
#define GD32_GPIO_TO_PORT(g) gd32::GpioToPort((g))
#define GD32_GPIO_TO_NUMBER(g) gd32::GpioToNumber((g))
#endif // __cplusplus

#endif // GD32_GPIO_MACROS_H_
