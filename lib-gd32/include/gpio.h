/**
 * @file gpio.h
 * @brief GD32
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

#ifndef GPIO_H_
#define GPIO_H_

#include <cstdint>

#include "gd32_gpio.h"
#include "gd32_board.h" // IWYU pragma: keep

namespace gpio {
enum class Select { 
  kInput = GPIO_FSEL_INPUT, 
  kOutput = GPIO_FSEL_OUTPUT 
};

enum class Pull { 
  kDisable = GPIO_PULL_DISABLE, 
  kUp = GPIO_PULL_UP, 
  kDown = GPIO_PULL_DOWN 
};

enum class IntConfig {
//  kPosEdge = 
//  kNegEdge = 
  kHighLev = GPIO_INT_CFG_NEG_EDGE,
//  kLowLev = GPIO_INT_CFG_LOW_LEV,
  kDoubleEdge = EXTI_TRIG_BOTH  
};

inline void Fsel(uint32_t gpio, Select fsel) {
    Gd32GpioFsel(gpio, static_cast<uint32_t>(fsel));
}

inline void SetPud(uint32_t gpio, Pull pull) {
    Gd32GpioSetPud(gpio, static_cast<uint32_t>(pull));
}

inline void IntCfg(uint32_t gpio, IntConfig int_cfg) {
    Gd32GpioIntCfg(gpio, static_cast<uint32_t>(int_cfg));
}

inline void Set(uint32_t pin) {
    Gd32GpioSet(pin);
}

inline void Clr(uint32_t pin) {
    Gd32GpioClr(pin);
}

inline void Write(uint32_t pin, uint32_t value) {
    if (value != 0) {
        Set(pin);
    } else {
        Clr(pin);
    }
}

inline uint8_t Lev(uint32_t pin) {
    return Gd32GpioLev(pin);
}
} // namespace gpio

#endif // GPIO_H_
