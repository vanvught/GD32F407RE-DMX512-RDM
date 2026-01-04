/**
 * @file hal_boardinfo.cpp
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

#include <cstdint>

#include "gd32_board.h"

namespace hal
{
const char* BoardName(uint8_t& length)
{
    length = sizeof(GD32_BOARD_NAME) - 1U;
    return GD32_BOARD_NAME;
}

const char* SocName(uint8_t& length)
{
    length = 4;
    return "GD32";
}

const char* CpuName(uint8_t& length)
{
    length = sizeof(GD32_MCU_NAME) - 1U;
    return GD32_MCU_NAME;
}

const char* SysName(uint8_t& length)
{
    length = 8;
    return "Embedded";
}
} // namespace hal
