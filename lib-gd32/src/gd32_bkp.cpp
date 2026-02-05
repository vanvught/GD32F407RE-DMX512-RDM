/**
 * @file gd32_bkp.cpp
 *
 */
/* Copyright (C) 2022-2026 by Arjan van Vught mailto:info@gd32-dmx.org
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

#if defined(GD32F4XX) || defined(GD32H7XX)
#include <cassert>
#include "gd32.h"

void bkp_data_write(bkp_data_register_enum register_number, uint16_t data)
{
    switch (register_number)
    {
        case BKP_DATA_0:
            RTC_BKP0 = static_cast<uint32_t>(data);
            break;
        case BKP_DATA_1:
            RTC_BKP1 = static_cast<uint32_t>(data);
            break;
        default:
            assert(false && "Invalid register_number");
            break;
    }
}

uint16_t bkp_data_read(bkp_data_register_enum register_number)
{
    switch (register_number)
    {
        case BKP_DATA_0:
            return RTC_BKP0;
            break;
        case BKP_DATA_1:
            return RTC_BKP1;
            break;
        default:
            assert(false && "Invalid register_number");
            break;
    }

    return 0;
}
#endif
