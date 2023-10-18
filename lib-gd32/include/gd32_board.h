/**
 * @file gd32_board.h
 *
 */
/* Copyright (C) 2021-2023 by Arjan van Vught mailto:info@gd32-dmx.org
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

#ifndef GD32_BOARD_H_
#define GD32_BOARD_H_

#if defined (BOARD_GD32F103RC)
# include "board/gd32f103rc.h"
#elif defined (BOARD_GD32F107RC)
# include "board/gd32f107rc.h"
#elif defined (BOARD_GD32F207RG)
# include "board/gd32f207rg.h"
#elif defined (BOARD_GD32F207VC_2)
# include "board/gd32f207vc_2.h"
#elif defined (BOARD_GD32F207VC_4)
# include "board/gd32f207vc_4.h"
#elif defined (BOARD_GD32F303RC)
# include "board/gd32f303rc.h"
#elif defined (BOARD_GD32F407RE)
# include "board/gd32f407re.h"
#elif defined (BOARD_GD32F450VE)
# include "board/gd32f450ve.h"
#elif defined (BOARD_GD32F450VI)
# include "board/gd32f450vi.h"
#elif defined (BOARD_16X4U_PIXEL)
# include "board/16x4u-pixel.h"
#elif defined (BOARD_GD32F207C_EVAL)
# include "board/gd32f207c_eval.h"
#elif defined (BOARD_BW_OPIDMX4)
# include "board/bw_opidmx4.h"
#elif defined (BOARD_DMX3)
# include "board/dmx3.h"
#elif defined (BOARD_DMX4)
# include "board/dmx4.h"
#else
# error Board is unknown / not defined
#endif

#include "board/logic_analyzer.h"

#if defined(USART0_REMAP) && !defined (I2C0_REMAP)
# error Configuration error
#endif

#endif /* GD32_BOARD_H_ */
