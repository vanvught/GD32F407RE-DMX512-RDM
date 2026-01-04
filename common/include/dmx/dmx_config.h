#ifndef DMX_DMX_CONFIG_H_
#define DMX_DMX_CONFIG_H_

/**
 * @file dmx_config.h
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

#include "gd32.h" // IWYU pragma: keep

namespace dmx::config
{
#if defined(BOARD_GD32F103RC)
#include "board_gd32f103rc.h"
#elif defined(BOARD_GD32F107RC)
#include "board_gd32f107rc.h"
#elif defined(BOARD_GD32F207RG)
#include "board_gd32f207rg.h"
#elif defined(BOARD_GD32F303RC)
#include "board_gd32f303rc.h"
#elif defined(BOARD_GD32F407RE)
#include "board_gd32f407re.h"
#elif defined(BOARD_GD32F450VI)
#include "board_gd32f450vi.h"
#elif defined(BOARD_GD32H757ZM)
#include "board_gd32h757zm.h"
#elif defined(BOARD_GD32F470Z_EVAL)
#include "board_gd32f470z_eval.h"
#elif defined(BOARD_GD32F207C_EVAL)
#include "board_gd32f207c_eval.h"
#elif defined(BOARD_GD32H759I_EVAL)
#include "board_gd32h759i_eval.h"
#elif defined(BOARD_BW_OPIDMX4)
#include "board_bw_opidmx4.h"
#elif defined(BOARD_DMX3)
#include "board_dmx3.h"
#elif defined(BOARD_DMX4)
#include "board_dmx4.h"
#else
#error
#endif
} // namespace dmx::config

namespace dmx::buffer
{
static constexpr auto SIZE = 516; // multiple of uint32_t
} // namespace dmx::buffer

#include "gd32/dmx_dma_check.h"

#endif  // DMX_DMX_CONFIG_H_
