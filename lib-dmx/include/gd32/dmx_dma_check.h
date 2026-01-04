/**
 * @file dmx_dma_check.h
 *
 */
/* Copyright (C) 2025 by Arjan van Vught mailto:info@gd32-dmx.org
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

#ifndef GD32_DMX_DMA_CHECK_H_
#define GD32_DMX_DMA_CHECK_H_

#if defined(GD32F10X_HD) || defined(GD32F10X_CL)
static_assert(DMX_MAX_PORTS <= 4, "Too many ports defined");
#endif
#if defined(GD32F20X_CL)
static_assert(DMX_MAX_PORTS <= 6, "Too many ports defined");
#endif

/**
 * DMA channel check
 */
#if defined(GD32F10X_HD) || defined(GD32F10X_CL)
#if defined(DMX_USE_UART4)
#error There is no DMA channel for UART4
#endif
#if defined(DMX_USE_USART5)
#error USART5 is not available
#endif
#if defined(DMX_USE_UART6)
#error UART6 is not available
#endif
#if defined(DMX_USE_UART7)
#error UART7 is not available
#endif
#endif

#if defined(GD32F20X_CL)
#if defined(DMX_USE_UART4) && defined(DMX_USE_UART7)
#error DMA1 Channel 3
#endif
#if defined(DMX_USE_UART3) && defined(DMX_USE_UART6)
#error DMA1 Channel 4
#endif
#endif

#endif // GD32_DMX_DMA_CHECK_H_
