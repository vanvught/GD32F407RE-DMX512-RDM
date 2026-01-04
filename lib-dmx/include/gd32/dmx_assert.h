/**
 * @file dmx_assert.h
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

#ifndef GD32_DMX_ASSERT_H_
#define GD32_DMX_ASSERT_H_

#include <cassert> // IWYU pragma: keep

#include "dmx/dmx_config.h" // IWYU pragma: keep

// For void-returning functions
#if defined(NDEBUG)
#define DMX_CHECK_PORT_INDEX_VOID(x) ((void)0)
#else
#define DMX_CHECK_PORT_INDEX_VOID(x)                     \
    do                                                   \
    {                                                    \
        assert((x) < dmx::config::max::PORTS);           \
        if ((x) >= dmx::config::max::PORTS) [[unlikely]] \
            return;                                      \
    } while (0)
#endif

// For functions that return a value (e.g., enum, int, etc.)
#if defined(NDEBUG)
#define DMX_CHECK_PORT_INDEX_RET(x, ret) ((void)0)
#else
#define DMX_CHECK_PORT_INDEX_RET(x, ret)                 \
    do                                                   \
    {                                                    \
        assert((x) < dmx::config::max::PORTS);           \
        if ((x) >= dmx::config::max::PORTS) [[unlikely]] \
            return ret;                                  \
    } while (0)
#endif

// For functions that return pointers
#if defined(NDEBUG)
#define DMX_CHECK_PORT_INDEX_PTR(x) ((void)0)
#else
#define DMX_CHECK_PORT_INDEX_PTR(x)                      \
    do                                                   \
    {                                                    \
        assert((x) < dmx::config::max::PORTS);           \
        if ((x) >= dmx::config::max::PORTS) [[unlikely]] \
            return nullptr;                              \
    } while (0)
#endif

#endif  // GD32_DMX_ASSERT_H_
