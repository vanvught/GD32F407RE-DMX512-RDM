/**
 * @file dmx_config.h
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

#ifndef DMX_DMX_CONFIG_H_
#define DMX_DMX_CONFIG_H_

#include "gd32.h" // IWYU pragma: keep

#if defined(BOARD_GD32F103RC)
#include "board_gd32f103rc.h" // IWYU pragma: keep
#elif defined(BOARD_GD32F107RC)
#include "board_gd32f107rc.h" // IWYU pragma: keep
#elif defined(BOARD_GD32F207RG)
#include "board_gd32f207rg.h" // IWYU pragma: keep
#elif defined(BOARD_GD32F303RC)
#include "board_gd32f303rc.h" // IWYU pragma: keep
#elif defined(BOARD_GD32F407RE)
#include "board_gd32f407re.h" // IWYU pragma: keep
#elif defined(BOARD_GD32F450VI)
#include "board_gd32f450vi.h" // IWYU pragma: keep
#elif defined(BOARD_GD32H757ZM)
#include "board_gd32h757zm.h" // IWYU pragma: keep
#elif defined(BOARD_GD32F470Z_EVAL)
#include "board_gd32f470z_eval.h" // IWYU pragma: keep
#elif defined(BOARD_GD32F207C_EVAL)
#include "board_gd32f207c_eval.h" // IWYU pragma: keep
#elif defined(BOARD_GD32H759I_EVAL)
#include "board_gd32h759i_eval.h" // IWYU pragma: keep
#elif defined(BOARD_BW_OPIDMX4)
#include "board_bw_opidmx4.h" // IWYU pragma: keep
#elif defined(BOARD_DMX3)
#include "board_dmx3.h" // IWYU pragma: keep
#elif defined(BOARD_DMX4)
#include "board_dmx4.h" // IWYU pragma: keep
#else
#error Board is not defined
#endif

namespace dmx::buffer {
static constexpr auto kSize = 516;
} // namespace dmx::buffer
static_assert(dmx::buffer::kSize >= 513);	// 512 with Start Code
static_assert(dmx::buffer::kSize % 4 == 0); // multiple of uint32_t

// Maximum available USART check
#if defined(GD32F10X_HD) || defined(GD32F10X_CL)
static_assert(DMX_MAX_PORTS <= 4, "Too many ports defined");
#endif
#if defined(GD32F20X_CL)
static_assert(DMX_MAX_PORTS <= 6, "Too many ports defined");
#endif
#if defined(GD32F30X_HD)
static_assert(DMX_MAX_PORTS <= 5, "Too many ports defined");
#endif

// DMA channel check
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

#if defined(GD32F30X_HD)
#if defined(DMX_USE_UART4)
#error There is no DMA channel for UART4
#endif
#endif

#endif // DMX_DMX_CONFIG_H_
