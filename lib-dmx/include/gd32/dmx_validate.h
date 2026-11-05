/**
 * @file dmx_validate.h
 *
 */
/* Copyright (C) 2026 by Arjan van Vught mailto:info@gd32-dmx.org
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of thnDmxDataDirecte Software, and to permit persons to whom the Software is
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

#ifndef GD32_DMX_VALIDATE_H_
#define GD32_DMX_VALIDATE_H_

#include <cstdint>

#include "gd32/dmx.h" // IWYU pragma: keep

namespace dmx::validate {
// Maximum available U(S)ART check
#if defined(GD32F10X_HD) || defined(GD32F10X_CL)
inline constexpr uint32_t kMaxPorts = 4;
#ifdef DMX_USE_USART5
#error USART5 is not available
#endif // DMX_USE_USART5
#ifdef DMX_USE_UART6
#error UART6 is not available
#endif // DMX_USE_UART6
#ifdef DMX_USE_UART7
#error UART7 is not available
#endif // DMX_USE_UART7
#endif // defined(GD32F10X_HD) || defined(GD32F10X_CL)

#ifdef GD32F20X_CL
#ifdef GD32F207RG
inline constexpr uint32_t kMaxPorts = 6;
#ifdef DMX_USE_UART6
#error UART6 is not available
#endif // DMX_USE_UART6
#ifdef DMX_USE_UART7
#error UART7 is not available
#endif // DMX_USE_UART7
#endif // GD32F207RG
#ifdef GD32F207VC
inline constexpr uint32_t kMaxPorts = 8;
#endif // GD32F207VC
#endif // GD32F20X_CL

#if defined(GD32F30X_HD) || defined(GD32F30X_XD)
    inline constexpr uint32_t kMaxPorts = 5;
#ifdef DMX_USE_USART5
#error USART5 is not available
#endif // DMX_USE_USART5
#ifdef DMX_USE_UART6
#error UART6 is not available
#endif // DMX_USE_UART6
#ifdef DMX_USE_UART7
#error UART7 is not available
#endif // DMX_USE_UART7
#endif // defined(GD32F30X_HD) || defined(GD32F30X_XD)

#if defined(GD32F407) || defined(GD32F427)
inline constexpr uint32_t kMaxPorts = 6;
#ifdef DMX_USE_UART6
#error UART6 is not available
#endif // DMX_USE_UART6
#ifdef DMX_USE_UART7
#error UART7 is not available
#endif // DMX_USE_UART7
#endif // defined(GD32F407) || defined(GD32F427)
#if defined(GD32F450) || defined(GD32F470)
inline constexpr uint32_t kMaxPorts = 8;
#endif // defined(GD32F450) || defined(GD32F470)

#if defined(GD32H757) || defined(GD32H759)
inline constexpr uint32_t kMaxPorts = 8;
#endif // defined(GD32H757) || defined(GD32H759)

// DMA channel check
#if defined(GD32F10X_HD) || defined(GD32F10X_CL)
#ifdef DMX_USE_UART4
#define UART4_NO_DMA_CHANNEL
#endif // DMX_USE_UART4
#endif // defined(GD32F10X_HD) || defined(GD32F10X_CL)

#ifdef GD32F20X_CL
#ifdef GD32F207VC
#if defined(DMX_USE_UART4) && defined(DMX_USE_UART7)
#error DMA1 Channel 3
#endif // defined(DMX_USE_UART4) && defined(DMX_USE_UART7)
#if defined(DMX_USE_UART3) && defined(DMX_USE_UART6)
#error DMA1 Channel 4
#endif // defined(DMX_USE_UART3) && defined(DMX_USE_UART6)
#endif // GD32F207VC
#endif // GD32F20X_CL

#if defined(GD32F30X_HD) || defined(GD32F30X_XD)
#ifdef DMX_USE_UART4
#define UART4_NO_DMA_CHANNEL
#endif // DMX_USE_UART4
#endif // defined(GD32F30X_HD) || defined(GD32F30X_XD)

static_assert(DMX_MAX_PORTS <= kMaxPorts, "Too many ports defined");
} // namespace dmx::validate

#endif // GD32_DMX_VALIDATE_H_
