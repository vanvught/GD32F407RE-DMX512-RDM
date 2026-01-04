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

#include "configstoredevice.h"
#include "flashcode.h"
#include "firmware/debug/debug_debug.h"

StoreDevice::StoreDevice()
{
    DEBUG_ENTRY();

    detected_ = FlashCode::IsDetected();

    DEBUG_EXIT();
}

StoreDevice::~StoreDevice()
{
  DEBUG_ENTRY();
  DEBUG_EXIT();
}

uint32_t StoreDevice::GetSize() const
{
    return FlashCode::GetSize();
}

uint32_t StoreDevice::GetSectorSize() const
{
    return FlashCode::GetSectorSize();
}

bool StoreDevice::Read(uint32_t offset, uint32_t length, uint8_t* buffer, storedevice::Result& result)
{
    DEBUG_ENTRY();

    flashcode::Result flashrom_result;
    const auto kState = FlashCode::Read(offset, length, buffer, flashrom_result);

    result = static_cast<storedevice::Result>(flashrom_result);

    DEBUG_EXIT();
    return kState;
}

bool StoreDevice::Erase(uint32_t offset, uint32_t length, storedevice::Result& result)
{
    DEBUG_ENTRY();

    flashcode::Result flashrom_result;
    const auto kState = FlashCode::Erase(offset, length, flashrom_result);

    result = static_cast<storedevice::Result>(flashrom_result);

    DEBUG_EXIT();
    return kState;
}

bool StoreDevice::Write(uint32_t offset, uint32_t length, const uint8_t* buffer, storedevice::Result& result)
{
    DEBUG_ENTRY();

    flashcode::Result flashrom_result;
    const auto kState = FlashCode::Write(offset, length, buffer, flashrom_result);

    result = static_cast<storedevice::Result>(flashrom_result);

    DEBUG_EXIT();
    return kState;
}
