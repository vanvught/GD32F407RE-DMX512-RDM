/**
 * @file hal_panelled.h
 *
 */
/* Copyright (C) 2023-2025 by Arjan van Vught mailto:info@gd32-dmx.org
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

#ifndef GD32_HAL_PANELLED_H_
#define GD32_HAL_PANELLED_H_

#include <cstdint>

#include "gd32.h"

namespace hal::panelled
{
namespace global
{
extern uint32_t data;
}

inline void LedSpi([[maybe_unused]] uint32_t data)
{
#if defined(PANELLED_595_COUNT)
    GPIO_BC(PANELLED_595_CS_GPIOx) = PANELLED_595_CS_GPIO_PINx;

#if (PANELLED_595_COUNT >= 1)
    while (RESET == (SPI_STAT(SPI_PERIPH) & SPI_FLAG_TBE));

    SPI_DATA(SPI_PERIPH) = (data & 0xFF);

    while (RESET == (SPI_STAT(SPI_PERIPH) & SPI_FLAG_RBNE));

    static_cast<void>(SPI_DATA(SPI_PERIPH));
#endif
#if (PANELLED_595_COUNT >= 2)
    while (RESET == (SPI_STAT(SPI_PERIPH) & SPI_FLAG_TBE));

    SPI_DATA(SPI_PERIPH) = ((data >> 8) & 0xFF);

    while (RESET == (SPI_STAT(SPI_PERIPH) & SPI_FLAG_RBNE));

    static_cast<void>(SPI_DATA(SPI_PERIPH));
#endif
#if (PANELLED_595_COUNT >= 3)
    while (RESET == (SPI_STAT(SPI_PERIPH) & SPI_FLAG_TBE));

    SPI_DATA(SPI_PERIPH) = ((data >> 16) & 0xFF);

    while (RESET == (SPI_STAT(SPI_PERIPH) & SPI_FLAG_RBNE));

    static_cast<void>(SPI_DATA(SPI_PERIPH));
#endif
#if (PANELLED_595_COUNT == 4)
    while (RESET == (SPI_STAT(SPI_PERIPH) & SPI_FLAG_TBE));

    SPI_DATA(SPI_PERIPH) = ((data >> 24) & 0xFF);

    while (RESET == (SPI_STAT(SPI_PERIPH) & SPI_FLAG_RBNE));

    static_cast<void>(SPI_DATA(SPI_PERIPH));
#endif

    GPIO_BOP(PANELLED_595_CS_GPIOx) = PANELLED_595_CS_GPIO_PINx;
#endif
}

inline void Init()
{
#if defined(PANELLED_595_COUNT)

#endif
}

inline void On([[maybe_unused]] uint32_t on)
{
#if defined(PANELLED_595_COUNT)
    if (global::data == (global::data | on))
    {
        return;
    }

    global::data |= on;

    LedSpi(global::data);
#endif
}

inline void Off([[maybe_unused]] uint32_t off)
{
#if defined(PANELLED_595_COUNT)
    if (global::data == (global::data & ~off))
    {
        return;
    }

    global::data &= ~off;

    LedSpi(global::data);
#endif
}

inline void Run()
{
#if defined(PANELLED_595_COUNT)

#endif
}
} // namespace hal::panelled

#endif // GD32_HAL_PANELLED_H_
