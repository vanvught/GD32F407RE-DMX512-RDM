/**
 * @file gd32_debug.h
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

#ifndef GD32_DEBUG_H_
#define GD32_DEBUG_H_

#include "firmware/debug/debug_debug.h"

#ifdef DEBUG_GD32_TIMERS
#define GD32_TIMERS_DEBUG_ENTRY() DEBUG_ENTRY()
#define GD32_TIMERS_DEBUG_EXIT() DEBUG_EXIT()
#define GD32_TIMERS_DEBUG_PRINTF(...) DEBUG_PRINTF(__VA_ARGS__)
#define GD32_TIMERS_DEBUG_PUTS(...) DEBUG_PUTS(__VA_ARGS__)
#else
#define GD32_TIMERS_DEBUG_ENTRY() \
    do {                          \
    } while (false)
#define GD32_TIMERS_DEBUG_EXIT() \
    do {                         \
    } while (false)
#define GD32_TIMERS_DEBUG_PRINTF(...) \
    do {                              \
    } while (false)
#define GD32_TIMERS_DEBUG_PUTS(...) \
    do {                            \
    } while (false)
#endif // DEBUG_GD32_TIMERS

#ifdef DEBUG_GD32_PWM
#define GD32_PWM_DEBUG_ENTRY() DEBUG_ENTRY()
#define GD32_PWM_DEBUG_EXIT() DEBUG_EXIT()
#define GD32_PWM_DEBUG_PRINTF(...) DEBUG_PRINTF(__VA_ARGS__)
#define GD32_PWM_DEBUG_PUTS(...) DEBUG_PUTS(__VA_ARGS__)
#else
#define GD32_PWM_DEBUG_ENTRY() \
    do {                       \
    } while (false)
#define GD32_PWM_DEBUG_EXIT() \
    do {                      \
    } while (false)
#define GD32_PWM_DEBUG_PRINTF(...) \
    do {                           \
    } while (false)
#define GD32_PWM_DEBUG_PUTS(...) \
    do {                         \
    } while (false)
#endif // DEBUG_GD32_PWM

#ifdef DEBUG_GD32_USB
#define GD32_USB_DEBUG_ENTRY() DEBUG_ENTRY()
#define GD32_USB_DEBUG_EXIT() DEBUG_EXIT()
#define GD32_USB_DEBUG_PRINTF(...) DEBUG_PRINTF(__VA_ARGS__)
#define GD32_USB_DEBUG_PUTS(...) DEBUG_PUTS(__VA_ARGS__)
#else
#define GD32_USB_DEBUG_ENTRY() \
    do {                       \
    } while (false)
#define GD32_USB_DEBUG_EXIT() \
    do {                      \
    } while (false)
#define GD32_USB_DEBUG_PRINTF(...) \
    do {                           \
    } while (false)
#define GD32_USB_DEBUG_PUTS(...) \
    do {                         \
    } while (false)
#endif // DEBUG_GD32_USB

#ifdef DEBUG_GD32_TRNG
#define GD32_TRNG_DEBUG_ENTRY() DEBUG_ENTRY()
#define GD32_TRNG_DEBUG_EXIT() DEBUG_EXIT()
#define GD32_TRNG_DEBUG_PRINTF(...) DEBUG_PRINTF(__VA_ARGS__)
#define GD32_TRNG_DEBUG_PUTS(...) DEBUG_PUTS(__VA_ARGS__)
#else
#define GD32_TRNG_DEBUG_ENTRY() \
    do {                       \
    } while (false)
#define GD32_TRNG_DEBUG_EXIT() \
    do {                      \
    } while (false)
#define GD32_TRNG_DEBUG_PRINTF(...) \
    do {                           \
    } while (false)
#define GD32_TRNG_DEBUG_PUTS(...) \
    do {                         \
    } while (false)
#endif // DEBUG_GD32_TRNG

#ifdef DEBUG_GD32_FMC
#define GD32_FMC_DEBUG_ENTRY() DEBUG_ENTRY()
#define GD32_FMC_DEBUG_EXIT() DEBUG_EXIT()
#define GD32_FMC_DEBUG_PRINTF(...) DEBUG_PRINTF(__VA_ARGS__)
#define GD32_FMC_DEBUG_PUTS(...) DEBUG_PUTS(__VA_ARGS__)
#else
#define GD32_FMC_DEBUG_ENTRY() \
    do {                  \
    } while (false)
#define GD32_FMC_DEBUG_EXIT() \
    do {                 \
    } while (false)
#define GD32_FMC_DEBUG_PRINTF(...) \
    do {                      \
    } while (false)
#define GD32_FMC_DEBUG_PUTS(...) \
    do {                    \
    } while (false)
#endif // DEBUG_COMMON_FMC

#endif // GD32_DEBUG_H_
