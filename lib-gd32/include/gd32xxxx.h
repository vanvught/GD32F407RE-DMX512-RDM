/**
 * @file gd32xxxx.h
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

#ifndef GD32FXXX_H_
#define GD32FXXX_H_

// Needed for GD32 Firmware and CMSIS

#ifdef __cplusplus
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wuseless-cast"
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#if __cplusplus > 201402
// error: compound assignment with 'volatile'-qualified left operand is
// deprecated
#pragma GCC diagnostic ignored "-Wvolatile"
#endif
#endif

#if defined(GD32F10X_HD) || defined(GD32F10X_CL)
#include "gd32f10x.h" // IWYU pragma: keep
#elif defined(GD32F20X_CL)
#include "gd32f20x.h" // IWYU pragma: keep
#elif defined(GD32F30X_HD)
#include "gd32f30x.h" // IWYU pragma: keep
#elif defined(GD32F407) || defined(GD32F450) || defined(GD32F470)
#include "gd32f4xx.h" // IWYU pragma: keep
#elif defined(GD32H757) || defined(GD32H759)
#pragma GCC diagnostic ignored "-Wunused-parameter"
#include "gd32h7xx.h" // IWYU pragma: keep
#else
#error MCU is not supported
#endif

#ifdef __cplusplus
#pragma GCC diagnostic pop
#endif

#endif // GD32FXXX_H_ */
