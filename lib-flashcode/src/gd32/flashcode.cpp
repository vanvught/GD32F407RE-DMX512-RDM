/**
 * @file flashcode.cpp
 *
 */
/* Copyright (C) 2021-2065 by Arjan van Vught mailto:info@gd32-dmx.org
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
 #include <cassert>
 #include <span>

 #include "flashcode.h"
 #include "gd32_fmc.h"
 #include "gd32.h" // IWYU pragma: keep
 
 namespace {
 constexpr uint32_t k1KiB = 1024;
 // Backwards compatibility with SPI FLASH
 constexpr auto kFlashSectorSize = 4096;
 } // namespace

FlashCode::FlashCode() {
    FLASHCODE_DEBUG_ENTRY();
    assert(s_this == nullptr);
    s_this = this;

    detected_ = true;

    printf("FMC: %s %u [%u]\n", GetName(), static_cast<unsigned>(GetSize()), static_cast<unsigned>(GetSize() / k1KiB));
    FLASHCODE_DEBUG_EXIT();
}

const char* FlashCode::GetName() const {
    return GD32_MCU_NAME;
}

uint32_t FlashCode::GetSize() const {
    return FMC_SIZE * k1KiB;
}

uint32_t FlashCode::GetSectorSize() const {
    return kFlashSectorSize;
}

bool FlashCode::Read(uint32_t offset, std::span<uint8_t> buffer, flashcode::Result& result) {
    const auto kStatus = gd32::fmc::Read(offset, buffer); // Blocking
    result = kStatus ? flashcode::Result::kOk : flashcode::Result::kError;
    return true;
}

bool FlashCode::Erase(uint32_t offset, uint32_t length, flashcode::Result& result) {
    gd32::fmc::Result fmc_result;
    const auto kStatus = gd32::fmc::Erase(offset, length, fmc_result); // State-machine
    result = (fmc_result == gd32::fmc::Result::kOk) ? flashcode::Result::kOk : flashcode::Result::kError;
    return kStatus;
}

bool FlashCode::Write(uint32_t offset, std::span<const uint8_t> buffer, flashcode::Result& result) {
    gd32::fmc::Result fmc_result;
    const auto kStatus = gd32::fmc::Write(offset, buffer, fmc_result); // State-machine
    result = (fmc_result == gd32::fmc::Result::kOk) ? flashcode::Result::kOk : flashcode::Result::kError;
    return kStatus;
}
