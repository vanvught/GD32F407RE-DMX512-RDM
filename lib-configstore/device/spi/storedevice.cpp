/**
 * @file storedevice.cpp
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

#if defined(CONFIG_STORE_USE_I2C) || defined(CONFIG_STORE_USE_ROM) || defined(CONFIG_STORE_USE_RAM)
#error Configuration error
#endif

#include <cstdint>
#include <cstdio>
#include <span>

#include "configstoredevice.h"
#include "spi/spi_flash.h"
#include "configstore_debug.h"

StoreDevice::StoreDevice() {
    CONFIGSTORE_DEBUG_ENTRY();

    if (!spi::flash::Probe()) {
        puts("StoreDevice: No SPI flash chip.");
    } else {
        printf("StoreDevice: SPI flash %s sector size %u total %u bytes [%u kB]\n", spi::flash::Name(), static_cast<unsigned int>(spi::flash::SectorSize()), static_cast<unsigned int>(spi::flash::Size()),
               static_cast<unsigned int>(spi::flash::Size() / 1024U));
        detected_ = true;
    }

    CONFIGSTORE_DEBUG_EXIT();
}

StoreDevice::~StoreDevice() {
    CONFIGSTORE_DEBUG_ENTRY();
    CONFIGSTORE_DEBUG_EXIT();
}

uint32_t StoreDevice::GetSize() const {
    return spi::flash::Size();
}

uint32_t StoreDevice::GetSectorSize() const {
    return spi::flash::SectorSize();
}

bool StoreDevice::Read(uint32_t offset, std::span<uint8_t> buffer, storedevice::Result& result) {
    CONFIGSTORE_DEBUG_ENTRY();

    result = spi::flash::cmd::Read(offset, buffer) ? storedevice::Result::kOk : storedevice::Result::kError;

    CONFIGSTORE_DEBUG_PRINTF("result=%d", static_cast<int>(result));
    CONFIGSTORE_DEBUG_EXIT();
    return true;
}

bool StoreDevice::Erase(uint32_t offset, uint32_t length, storedevice::Result& result) {
    CONFIGSTORE_DEBUG_ENTRY();

    result = spi::flash::cmd::Erase(offset, length) ? storedevice::Result::kOk : storedevice::Result::kError;

    CONFIGSTORE_DEBUG_PRINTF("result=%d", static_cast<int>(result));
    CONFIGSTORE_DEBUG_EXIT();
    return true;
}

bool StoreDevice::Write(uint32_t offset, std::span<const uint8_t> buffer, storedevice::Result& result) {
    CONFIGSTORE_DEBUG_ENTRY();

    result = spi::flash::cmd::Write(offset, buffer) ? storedevice::Result::kOk : storedevice::Result::kError;

    CONFIGSTORE_DEBUG_PRINTF("result=%d", static_cast<int>(result));
    CONFIGSTORE_DEBUG_EXIT();
    return true;
}
