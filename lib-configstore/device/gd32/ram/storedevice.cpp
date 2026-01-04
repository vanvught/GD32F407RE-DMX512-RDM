/**
 * @file storedevice.cpp
 *
 */
/* Copyright (C) 2022-2025 by Arjan van Vught mailto:info@gd32-dmx.org
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
#include <cstdio>
#include <cassert>

#include "configstoredevice.h"
#include "gd32.h"

#include "firmware/debug/debug_debug.h"

static constexpr uint32_t kFlashSectorSize = 4096U;
static constexpr uint32_t kBsramSize = 4096U;

StoreDevice::StoreDevice()
{
    DEBUG_ENTRY();

    detected_ = true;

    printf("StoreDevice: BSRAM with total %d bytes [%d kB]\n", GetSize(), GetSize() / 1024U);
    DEBUG_EXIT();
}

StoreDevice::~StoreDevice(){DEBUG_ENTRY();

                                DEBUG_EXIT();}

uint32_t StoreDevice::GetSize() const
{
    return kBsramSize;
}

uint32_t StoreDevice::GetSectorSize() const
{
    return kFlashSectorSize;
}

bool StoreDevice::Read(__attribute__((unused)) uint32_t offset, __attribute__((unused)) uint32_t length, __attribute__((unused)) uint8_t* buffer, storedevice::Result& result)
{
    DEBUG_ENTRY();
    DEBUG_PRINTF("offset=%p[%d], len=%u[%d], data=%p[%d]", offset, (((uint32_t)(offset) & 0x3) == 0), length, (((uint32_t)(length) & 0x3) == 0), buffer, (((uint32_t)(buffer) & 0x3) == 0));
    assert((offset + length) <= BSRAM_SIZE);

    result = storedevice::Result::kOk;

    DEBUG_EXIT();
    return true;
}

bool StoreDevice::Erase(__attribute__((unused)) uint32_t offset, __attribute__((unused)) uint32_t length, storedevice::Result& result)
{
    DEBUG_ENTRY();

    result = storedevice::Result::kOk;

    DEBUG_EXIT();
    return true;
}

bool StoreDevice::Write(__attribute__((unused)) uint32_t offset, __attribute__((unused)) uint32_t length, __attribute__((unused)) const uint8_t* buffer, storedevice::Result& result)
{
    DEBUG_ENTRY();
    DEBUG_PRINTF("offset=%p[%d], len=%u[%d], data=%p[%d]", offset, (((uint32_t)(offset) & 0x3) == 0), length, (((uint32_t)(length) & 0x3) == 0), buffer, (((uint32_t)(buffer) & 0x3) == 0));
    assert((offset + length) <= BSRAM_SIZE);

    result = storedevice::Result::kOk;

    DEBUG_EXIT();
    return true;
}
