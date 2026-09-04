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

// With the latest GD32F firmware, this function is declared as static.
#if defined(GD32F20X)
extern "C" {
fmc_state_enum fmc_bank0_state_get(); // NOLINT
fmc_state_enum fmc_bank1_state_get(); // NOLINT
}
#endif

namespace {
constexpr uint32_t k1KiB = 1024;
// Backwards compatibility with SPI FLASH
constexpr auto kFlashSectorSize = 4096U;
// The flash page size is 2KB for bank0
constexpr auto kBanK0FlashPage = 2 * k1KiB;
// The flash page size is 4KB for bank1
constexpr auto kBanK1FlashPage = 4 * k1KiB;

constexpr uint32_t kStartAddress = FLASH_BASE;
constexpr uint32_t kBank0StartAddress = kStartAddress;
constexpr uint32_t kBank1StartAddress = 0x08080000;
const uint32_t kEndAddress = (kStartAddress + (FMC_SIZE * k1KiB) - 1);

enum class State { kIdle, kEraseBusy, kEraseProgram, kWriteBusy, kWriteProgram };

State s_state = State::kIdle;
uint32_t s_length;
uint32_t s_address;
const uint32_t* s_data;

bool IsBank0(uint32_t page_address) {
    // flash size is greater than 512k
    if (FMC_BANK0_SIZE < FMC_SIZE) {
        return FMC_BANK0_END_ADDRESS > page_address;
    }

    return true;
}

void Unlock(uint32_t page_address) {
    if (IsBank0(page_address)) {
        fmc_bank0_unlock();
    } else {
        fmc_bank1_unlock();
    }
}

void Lock(uint32_t page_address) {
    if (IsBank0(page_address)) {
        fmc_bank0_lock();
    } else {
        fmc_bank1_lock();
    }
}

bool IsBusy(uint32_t page_address) {
    if (IsBank0(page_address)) {
        return FMC_BUSY == fmc_bank0_state_get();
    }

    return FMC_BUSY == fmc_bank1_state_get();
}

void EnableProgram(uint32_t page_address) {
    if (IsBank0(page_address)) {
        FMC_CTL0 |= FMC_CTL0_PG;
    } else {
        FMC_CTL1 |= FMC_CTL1_PG;
    }
}

void DisableProgram(uint32_t page_address) {
    if (IsBank0(page_address)) {
        FMC_CTL0 &= ~FMC_CTL0_PG;
    } else {
        FMC_CTL1 &= ~FMC_CTL1_PG;
    }
}

void DisableErase(uint32_t page_address) {
    if (IsBank0(page_address)) {
        FMC_CTL0 &= ~FMC_CTL0_PER;
    } else {
        FMC_CTL1 &= ~FMC_CTL1_PER;
    }
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

    Result result;
    while (!Erase(offset, length, result)) {
    }

    GD32_FMC_DEBUG_EXIT();
    return result == Result::kOk;
}

bool Write(uint32_t offset, std::span<const uint8_t> buffer) {
    GD32_FMC_DEBUG_ENTRY();

    Result result;
    while (!Write(offset, buffer, result)) {
    }

    GD32_FMC_DEBUG_EXIT();
    return result == Result::kOk;
}

// State-machine API's
bool Erase(uint32_t offset, uint32_t length, Result& result) {
    GD32_FMC_DEBUG_ENTRY();
    GD32_FMC_DEBUG_PRINTF("State=%d", static_cast<int>(s_state));

    result = Result::kOk;

    switch (s_state) {
        case State::kIdle:
            GD32_FMC_DEBUG_ENTRY();
            GD32_FMC_DEBUG_PUTS("State::IDLE");

            s_address = offset + FLASH_BASE;
            s_length = length;
            Unlock(s_address);
            s_state = State::kEraseBusy;

            GD32_FMC_DEBUG_PRINTF("IsBank0=%d", static_cast<int>(IsBank0(s_address)));
            return false;

        case State::kEraseBusy:
            if (IsBusy(s_address)) {
                return false;
            }

            DisableErase(s_address);

            if (s_length == 0) {
                Lock(s_address);
                s_state = State::kIdle;

                GD32_FMC_DEBUG_EXIT();
                return true; // Ending state-machine
            }

            s_state = State::kEraseProgram;
            return false;

        case State::kEraseProgram:
            if (s_length > 0) {
                GD32_FMC_DEBUG_PRINTF("s_address=%p", reinterpret_cast<void*>(s_address));

                if (IsBank0(s_address)) {
                    FMC_CTL0 |= FMC_CTL0_PER;
                    FMC_ADDR0 = s_address;
                    FMC_CTL0 |= FMC_CTL0_START;

                    s_length -= kBanK0FlashPage;
                    s_address += kBanK0FlashPage;
                } else {
                    FMC_CTL1 |= FMC_CTL1_PER;
                    FMC_ADDR1 = s_address;
                    if (FMC_OBSTAT & FMC_OBSTAT_SPC) {
                        FMC_ADDR0 = s_address;
                    }
                    FMC_CTL1 |= FMC_CTL1_START;

                    s_length -= kBanK1FlashPage;
                    s_address += kBanK1FlashPage;
                }
            }

            s_state = State::kEraseBusy;
            return false;

        case State::kWriteBusy:
            DisableProgram(s_address);
            /*@fallthrough@*/
            /* no break */
        case State::kWriteProgram:
            s_state = State::kIdle;
            return false;

        default:
            assert(0);
            __builtin_unreachable();
            break;
    }

    assert(0);
    __builtin_unreachable();
    return true;
}

bool Write(uint32_t offset, std::span<const uint8_t> buffer, Result& result) {
    result = Result::kOk;

    switch (s_state) {
        case State::kIdle: {
            GD32_FMC_DEBUG_ENTRY();
            GD32_FMC_DEBUG_PUTS("State::IDLE");

            const auto kAddress = offset + FLASH_BASE;

            if (buffer.empty() || ((buffer.size() % sizeof(uint32_t)) != 0) || (kAddress < kStartAddress) || (kAddress >= kEndAddress) || (buffer.size() > (kEndAddress - kAddress))) {
                result = Result::kError;
                GD32_FMC_DEBUG_EXIT();
                return true; // Ending state-machine
            }

            assert((reinterpret_cast<uintptr_t>(buffer.data()) % alignof(uint32_t)) == 0);

            s_address = kAddress;
            s_data = reinterpret_cast<const uint32_t*>(buffer.data());
            s_length = static_cast<uint32_t>(buffer.size());
            
            Unlock(s_address);
            
            s_state = State::kWriteProgram;

            GD32_FMC_DEBUG_PRINTF("IsBank0=%d", static_cast<int>(IsBank0(s_address)));
            return false;
        }

        case State::kWriteBusy:
            if (IsBusy(s_address)) {
                return false;
            }

            DisableProgram(s_address);

            if (s_length == 0) {
                Lock(s_address);
                s_state = State::kIdle;
                
                GD32_FMC_DEBUG_EXIT();
                return true; // Ending state-machine
            }

            s_address += sizeof(uint32_t);
            s_state = State::kWriteProgram;
            return false;

        case State::kWriteProgram:
            EnableProgram(s_address);

            REG32(s_address) = *s_data++;

            s_length -= sizeof(uint32_t);

            s_state = State::kWriteBusy;
            return false;

        case State::kEraseBusy:
            DisableErase(s_address);
            /*@fallthrough@*/
            /* no break */

        case State::kEraseProgram:
            s_state = State::kIdle;
            return false;

        default:
            assert(0);
            __builtin_unreachable();
            break;
    }

    assert(0);
    __builtin_unreachable();
    return true;
}
} // namespace gd32::fmc