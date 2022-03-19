/**
 * @file gd32.h
 *
 */
/* Copyright (C) 2021-2022 by Arjan van Vught mailto:info@gd32-dmx.org
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

#ifndef GD32_H_
#define GD32_H_

#include <stdint.h>

#if !defined  __cplusplus
 void udelay(uint32_t us);
#else
# if !defined(GD32_UDELAY)
#  define GD32_UDELAY
 void udelay(uint32_t us, uint32_t offset = 0);
# endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

#if defined  (GD32F10X_HD) || defined (GD32F10X_CL)
# define GD32F10X
# include "gd32f10x.h"
# include "gd32f10x_libopt.h"
#elif defined (GD32F20X_CL)
# define GD32F20X
# include "gd32f20x.h"
# include "gd32f20x_libopt.h"
#elif defined (GD32F30X_HD)
# define GD32F30X
# include "gd32f30x.h"
# include "gd32f30x_libopt.h"
#elif defined  (GD32F407)
# define GD32F4XX
# include "gd32f4xx.h"
# include "gd32f4xx_libopt.h"
#else
# error MCU is not supported
#endif

#ifdef __cplusplus
}
#endif

#if defined(GD32F30X)
# define bkp_data_write		bkp_write_data
# define bkp_data_read		bkp_read_data
#endif

#if defined(GD32F4XX) && defined(__cplusplus)
typedef enum
{
    BKP_DATA_0,
    BKP_DATA_1
}bkp_data_register_enum;
void bkp_data_write(bkp_data_register_enum register_number, uint16_t data);
uint16_t bkp_data_read(bkp_data_register_enum register_number);
#endif

#include "gd32_board.h"

#endif /* GD32_H_ */
