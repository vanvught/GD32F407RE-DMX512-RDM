/**
 * @file gd32_fmc.cpp
 *
 */
/* Copyright (C) 2021-2026 by Arjan van Vught mailto:info@gd32-dmx.org
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
#include <span>
#include <cassert>

#include "gd32_fmc.h"
#include "gd32_debug.h"
#include "gd32.h" // IWYU pragma: keep

namespace {
constexpr uint32_t kSize16Kb = 0x00004000;
constexpr uint32_t kSize64Kb = 0x00010000;
constexpr uint32_t kSize128Kb = 0x00020000;
constexpr uint32_t kSize256Kb = 0x00040000;

constexpr uint32_t kStartAddress = FLASH_BASE;
constexpr uint32_t kFmcBanK0StartAddress = kStartAddress;
constexpr uint32_t kFmcBanK1StartAddress = 0x08100000;
const auto kEndAddress = (kStartAddress + (FMC_SIZE * 1024) - 1);
constexpr uint32_t kFmcMaxEndAddress = 0x08300000U;

constexpr uint32_t kFmcWrongSectorName = 0xFFFFFFFF; // wrong sector name
constexpr uint32_t kFmcWrongSectorNum = 0xFFFFFFFF;  // wrong sector number
constexpr uint32_t kFmcInvalidSize = 0xFFFFFFFF;     // invalid sector size
constexpr uint32_t kFmcInvalidAddr = 0xFFFFFFFF;     // invalid sector address

struct SectorInfo {
    uint32_t name;
    uint32_t number;
    uint32_t size;
    uint32_t start_address;
    uint32_t end_address;
};

struct SectorInfo SectorInfoGet(uint32_t addr) {
    struct SectorInfo sector_info;
    uint32_t temp = 0x00000000;

    if ((kStartAddress <= addr) && (kEndAddress >= addr)) {
        if ((kFmcBanK1StartAddress > addr)) {
            // bank0 area
            temp = (addr - kFmcBanK0StartAddress) / kSize16Kb;
            if (4U > temp) {
                sector_info.name = temp;
                sector_info.number = CTL_SN(temp);
                sector_info.size = kSize16Kb;
                sector_info.start_address = kFmcBanK0StartAddress + (kSize16Kb * temp);
                sector_info.end_address = sector_info.start_address + kSize16Kb - 1;
            } else if (8U > temp) {
                sector_info.name = 0x00000004U;
                sector_info.number = CTL_SN(4);
                sector_info.size = kSize64Kb;
                sector_info.start_address = 0x08010000;
                sector_info.end_address = 0x0801FFFF;
            } else {
                temp = (addr - kFmcBanK0StartAddress) / kSize128Kb;
                sector_info.name = (temp + 4);
                sector_info.number = CTL_SN(temp + 4);
                sector_info.size = kSize128Kb;
                sector_info.start_address = kFmcBanK0StartAddress + (kSize128Kb * temp);
                sector_info.end_address = sector_info.start_address + kSize128Kb - 1;
            }
        } else {
            // bank1 area
            temp = (addr - kFmcBanK1StartAddress) / kSize16Kb;
            if (4U > temp) {
                sector_info.name = (temp + 12);
                sector_info.number = CTL_SN(temp + 16);
                sector_info.size = kSize16Kb;
                sector_info.start_address = kFmcBanK0StartAddress + (kSize16Kb * temp);
                sector_info.end_address = sector_info.start_address + kSize16Kb - 1;
            } else if (8U > temp) {
                sector_info.name = 0x00000010;
                sector_info.number = CTL_SN(20);
                sector_info.size = kSize64Kb;
                sector_info.start_address = 0x08110000;
                sector_info.end_address = 0x0811FFFF;
            } else if (64U > temp) {
                temp = (addr - kFmcBanK1StartAddress) / kSize128Kb;
                sector_info.name = (temp + 16);
                sector_info.number = CTL_SN(temp + 20);
                sector_info.size = kSize128Kb;
                sector_info.start_address = kFmcBanK1StartAddress + (kSize128Kb * temp);
                sector_info.end_address = sector_info.start_address + kSize128Kb - 1;
            } else {
                temp = (addr - kFmcBanK1StartAddress) / kSize256Kb;
                sector_info.name = (temp + 20);
                sector_info.number = CTL_SN(temp + 8);
                sector_info.size = kSize256Kb;
                sector_info.start_address = kFmcBanK1StartAddress + (kSize256Kb * temp);
                sector_info.end_address = sector_info.start_address + kSize256Kb - 1;
            }
        }
    } else {
        // invalid address
        sector_info.name = kFmcWrongSectorName;
        sector_info.number = kFmcWrongSectorNum;
        sector_info.size = kFmcInvalidSize;
        sector_info.start_address = kFmcInvalidAddr;
        sector_info.end_address = kFmcInvalidAddr;
    }

    return sector_info;
}
} // namespace

namespace gd32::fmc {
// Blocking API's
bool Read(uint32_t offset, std::span<uint8_t> buffer) {
    GD32_FMC_DEBUG_ENTRY();

    const auto kAddress = offset + FLASH_BASE;

    if (buffer.empty() || ((buffer.size() % sizeof(uint32_t)) != 0) || (kAddress < kStartAddress) || (kAddress >= kEndAddress) || (buffer.size() > (kEndAddress - kAddress))) {
        return false;
    }

    assert((reinterpret_cast<uintptr_t>(buffer.data()) % alignof(uint32_t)) == 0);

    GD32_FMC_DEBUG_PRINTF("offset=%x, length=%u, data=%p", static_cast<unsigned>(offset), buffer.size(), buffer.data());

    const auto* src = reinterpret_cast<const uint32_t*>(kAddress);
    auto* dst = reinterpret_cast<uint32_t*>(buffer.data());

    auto length = buffer.size();

    while (length != 0) {
        *dst++ = *src++;
        length -= sizeof(uint32_t);
    }

    GD32_FMC_DEBUG_EXIT();
    return true;
}

bool Erase(uint32_t offset, uint32_t length) {
    GD32_FMC_DEBUG_ENTRY();

    struct SectorInfo sector_info;
    uint32_t address = offset + FLASH_BASE;

    auto size = static_cast<int>(length);

    while (size > 0) {
        sector_info = SectorInfoGet(address);

        if (kFmcWrongSectorName == sector_info.name) {
            GD32_FMC_DEBUG_EXIT();
            return true;
        }

        GD32_FMC_DEBUG_PRINTF("Address 0x%08X is located in the : SECTOR_NUMBER_%u", address, sector_info.name);
        GD32_FMC_DEBUG_PRINTF("Sector range: 0x%08X to 0x%08X", sector_info.start_address, sector_info.end_address);
        GD32_FMC_DEBUG_PRINTF("Sector size: %d KB", (sector_info. size / 1024));

        fmc_unlock();
        fmc_flag_clear(FMC_FLAG_END | FMC_FLAG_OPERR | FMC_FLAG_WPERR | FMC_FLAG_PGMERR | FMC_FLAG_PGSERR);

        if (FMC_READY != fmc_sector_erase(sector_info.number)) {
            GD32_FMC_DEBUG_EXIT();
            return false;
        }

        fmc_lock();

        size -= static_cast<int>(sector_info.size);
        address += sector_info.size;
    }

    GD32_FMC_DEBUG_EXIT();
    return true;
}

bool Write(uint32_t offset, std::span<const uint8_t> buffer) {
    GD32_FMC_DEBUG_ENTRY();
    const auto kAddress = offset + FLASH_BASE;

    if (buffer.empty() || ((buffer.size() % sizeof(uint32_t)) != 0) || (kAddress < kStartAddress) || (kAddress >= kEndAddress) || (buffer.size() > (kEndAddress - kAddress))) {
        return false;
    }

    assert((reinterpret_cast<uintptr_t>(buffer.data()) % alignof(uint32_t)) == 0);

    GD32_FMC_DEBUG_PRINTF("offset=%x, length=%u, data=%p", static_cast<unsigned>(offset), buffer.size(), buffer.data());

    fmc_unlock();
    fmc_flag_clear(FMC_FLAG_END | FMC_FLAG_OPERR | FMC_FLAG_WPERR | FMC_FLAG_PGMERR | FMC_FLAG_PGSERR);

    uint32_t address = kAddress;
    const auto* data = reinterpret_cast<const uint32_t*>(buffer.data());
    auto length = buffer.size();

    while (length >= sizeof(uint32_t)) {
        const auto kState = fmc_word_program(address, *data);

        if (FMC_READY != kState) {
            fmc_lock();
            GD32_FMC_DEBUG_PRINTF("kState=%d [%p]", kState, address);
            GD32_FMC_DEBUG_EXIT();
            return false;
        }

        ++data;
        address += sizeof(uint32_t);
        length -= sizeof(uint32_t);
    }

    if (length > 0) {
        const auto kState = fmc_word_program(address, *data);

        if (FMC_READY != kState) {
            fmc_lock();
            GD32_FMC_DEBUG_PRINTF("kState=%d [%p]", kState, address);
            GD32_FMC_DEBUG_EXIT();
            return false;
        }
    }

    fmc_lock();

    GD32_FMC_DEBUG_EXIT();
    return true;
}

// State-machine API's
bool Erase(uint32_t offset, uint32_t length, Result& result) {
    const auto kResult = Erase(offset, length);
    result = kResult ? Result::kOk : Result::kError;
    return true;
}

bool Write(uint32_t offset, std::span<const uint8_t> buffer, Result& result) {
    const auto kResult = Write(offset, buffer);
    result = kResult ? Result::kOk : Result::kError;
    return true;
}
} // namespace gd32::fmc