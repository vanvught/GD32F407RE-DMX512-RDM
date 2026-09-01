/**
 * @file spi_flash.cpp
 *
 */
/*
 * Original code : https://github.com/martinezjavier/u-boot/blob/master/drivers/mtd/spi/spi_flash.c
 */
/* Copyright (C) 2018-2026 by Arjan van Vught mailto:info@gd32-dmx.org
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

#include "spi/spi_flash.h"
#include "common/utils/utils_array.h"
#include "spi_flash_internal.h"
#include "firmware/debug/debug_dump.h"
#include "timing.h"
#include "common/utils/utils_math.h"
#include "watchdog.h"

namespace {
constexpr uint32_t kProgramTimeoutMillis = 200;                     // Maximum wait after PAGE PROGRAM.
constexpr uint32_t kSectorEraseTimeoutMillis = 400;                 // Maximum wait after SECTOR ERASE.
constexpr uint32_t kReadyTimeoutMillis = kSectorEraseTimeoutMillis; // Conservative wait when the preceding operation is unknown.

constexpr uint32_t kIdCodePartLength = 5;

constexpr uint8_t kCmdWriteStatus = 0x01;
constexpr uint8_t kCmdPageProgram = 0x02;
constexpr uint8_t kCmdWriteDisable = 0x04;
constexpr uint8_t kCmdReadStatus = 0x05;
constexpr uint8_t kCmdFlagStatus = 0x70;
constexpr uint8_t kCmdWriteEnable = 0x06;
constexpr uint8_t kCmdReadArrayFast = 0x0b;
constexpr uint8_t kCmdErase4K = 0x20;
constexpr uint8_t kCmdReadId = 0x9f;

struct spi::flash::Info s_flash = {.name = "", .size = 0, .poll_cmd = kCmdReadStatus};

constexpr struct {
    const uint8_t kIdcode;
    bool (*probe)(struct spi::flash::Info* flash, const uint8_t* idcode);
} kFlashes[] = {
// Keep it sorted by define name
#ifdef CONFIG_SPI_FLASH_GIGADEVICE
    {
        .kIdcode = 0xc8,
        .probe = spi::flash::ProbeGigadevice,
    },
#endif
#ifdef CONFIG_SPI_FLASH_MACRONIX
    {
        .kIdcode = 0xc2,
        .probe = spi::flash::ProbeMacronix,
    },
#endif
#ifdef CONFIG_SPI_FLASH_WINBOND
    {
        .kIdcode = 0xef,
        .probe = spi::flash::ProbeWinbond,
    },
#endif
};

#define IDCODE_LEN IDCODE_PART_LEN

uint32_t GetTimer(uint32_t base) {
    if (0 == base) {
        return timing::Millis();
    }

    return (timing::Millis()) - base;
}

void SpiFlashAddr(uint32_t address, uint8_t* command) {
    // cmd[0] is actual command
    command[1] = static_cast<uint8_t>(address >> 16);
    command[2] = static_cast<uint8_t>(address >> 8);
    command[3] = static_cast<uint8_t>(address >> 0);
}

void SpiFlashReadWrite(const uint8_t* command, uint32_t command_length, const uint8_t* data_out, uint8_t* data_in, uint32_t data_length) {
    uint32_t flags = SPI_XFER_BEGIN;

    if (data_length == 0) {
        flags |= SPI_XFER_END;
    }

    spi::flash::Transfer(command_length, command, nullptr, flags);

    if (data_length != 0) {
        spi::flash::Transfer(data_length, data_out, data_in, SPI_XFER_END);
    }
}

inline void SpiFlashCmdRead(const uint8_t* command, uint32_t command_length, uint8_t* data, uint32_t data_length) {
    SpiFlashReadWrite(command, command_length, nullptr, data, data_length);
}

inline void SpiFlashCmd(uint8_t command, uint8_t* response, uint32_t length) {
    SpiFlashCmdRead(&command, 1, response, length);
}

inline void SpiFlashCmdWrite(const uint8_t* command, uint32_t command_length, const uint8_t* data, uint32_t data_length) {
    SpiFlashReadWrite(command, command_length, data, nullptr, data_length);
}

inline void SpiFlashCmdWriteEnable() {
    SpiFlashCmd(kCmdWriteEnable, nullptr, 0);
}

bool SpiFlashCmdWaitReady(uint32_t timeout) {
    uint8_t cmd = kCmdReadStatus;

    spi::flash::Transfer(1, &cmd, nullptr, SPI_XFER_BEGIN);

    const auto kTimebase = GetTimer(0);
    uint8_t status;

    do {
        spi::flash::Transfer(1, nullptr, &status, 0);

        if ((status & STATUS_WIP) == 0) {
            break;
        }

    } while (GetTimer(kTimebase) < timeout);

    spi::flash::Transfer(0, nullptr, nullptr, SPI_XFER_END);

    if ((status & STATUS_WIP) == 0) {
        SPI_FLASH_DEBUG_PRINTF("get_timer(kTimebase)=%u", static_cast<unsigned>(GetTimer(kTimebase)));
        SPI_FLASH_DEBUG_EXIT();
        return true;
    }

    SPI_FLASH_DEBUG_PUTS("time out");
    SPI_FLASH_DEBUG_EXIT();
    return false;
}

bool SpiFlashWriteCommon(const uint8_t* command, uint32_t command_length, const uint8_t* data, uint32_t data_length, bool wait_ready) {
    const auto kTimeout = data == nullptr ? kSectorEraseTimeoutMillis : kProgramTimeoutMillis;

    SpiFlashCmdWriteEnable();
    SpiFlashCmdWrite(command, command_length, data, data_length);

    if (wait_ready) {
        const auto kRet = SpiFlashCmdWaitReady(kTimeout);

        if (!kRet) {
            SPI_FLASH_DEBUG_PRINTF("write %s timed out", data == nullptr ? "sector erase" : "program");
            return false;
        }
    }

    return true;
}

void SpiFlashReadCommon(const uint8_t* command, uint32_t command_length, uint8_t* data, uint32_t data_length) {
    SpiFlashCmdRead(command, command_length, data, data_length);
}
} // namespace

namespace spi::flash {
bool Probe() {
    spi::flash::Init();

    uint8_t idcode[kIdCodePartLength];
    SpiFlashCmd(kCmdReadId, idcode, sizeof(idcode));

    debug::Dump(idcode, sizeof(idcode));

    uint32_t index;

    for (index = 0; index < common::ArraySize(kFlashes); ++index) {
        if (kFlashes[index].kIdcode == idcode[0]) {
            if (kFlashes[index].probe(&s_flash, idcode)) {
                break;
            }
        }
    }

    if (index == common::ArraySize(kFlashes)) {
        SPI_FLASH_DEBUG_PRINTF("Unsupported manufacturer %02x", idcode[0]);
        return false;
    }

    SPI_FLASH_DEBUG_PRINTF("Detected %s total %u bytes", s_flash.name, static_cast<unsigned>(s_flash.size));

    return true;
}

uint32_t Size() {
    return s_flash.size;
}

const char* Name() {
    return s_flash.name;
}

namespace cmd {
bool Read(uint32_t offset, std::span<uint8_t> data) {
    SPI_FLASH_DEBUG_ENTRY();

    if (!SpiFlashCmdWaitReady(kReadyTimeoutMillis)) {
        SPI_FLASH_DEBUG_EXIT();
        return false;
    }

    uint8_t cmd[5];
    cmd[0] = kCmdReadArrayFast;
    cmd[4] = 0x00;

    while (!data.empty()) {
        watchdog::Feed();

        const auto kRemainLength = SPI_FLASH_16MB_BOUN - offset;
        const auto kReadLength = common::Min(static_cast<uint32_t>(data.size()), kRemainLength);

        SpiFlashAddr(offset, cmd);
        SpiFlashReadCommon(cmd, sizeof(cmd), data.data(), kReadLength);

        offset += kReadLength;
        data = data.subspan(kReadLength);
    }

    SPI_FLASH_DEBUG_EXIT();
    return true;
}

bool Write(uint32_t offset, std::span<const uint8_t> data) {
    SPI_FLASH_DEBUG_ENTRY();

    if (!SpiFlashCmdWaitReady(kReadyTimeoutMillis)) {
        SPI_FLASH_DEBUG_EXIT();
        return false;
    }

    uint8_t cmd[4];
    cmd[0] = kCmdPageProgram;

    while (!data.empty()) {
        watchdog::Feed();

        const auto kByteAddress = offset % spi::flash::kPageSize;
        const auto kChunkLength = common::Min(static_cast<uint32_t>(data.size()), spi::flash::kPageSize - kByteAddress);

        SpiFlashAddr(offset, cmd);

        const auto kRet = SpiFlashWriteCommon(cmd, sizeof(cmd), data.data(), kChunkLength, data.size() != kChunkLength);

        if (!kRet) {
            SPI_FLASH_DEBUG_PUTS("write failed");
            SPI_FLASH_DEBUG_EXIT();
            return false;
        }

        offset += kChunkLength;
        data = data.subspan(kChunkLength);
    }

    SPI_FLASH_DEBUG_EXIT();
    return true;
}

bool Erase(uint32_t offset, uint32_t length) {
    SPI_FLASH_DEBUG_ENTRY();

    if ((offset % spi::flash::kSectorSize) || (length % spi::flash::kSectorSize)) {
        SPI_FLASH_DEBUG_PUTS("Erase offset/length not multiple of erase size");
        SPI_FLASH_DEBUG_EXIT();
        return false;
    }

    if (!SpiFlashCmdWaitReady(kReadyTimeoutMillis)) {
        SPI_FLASH_DEBUG_EXIT();
        return false;
    }

    static_assert(spi::flash::kSectorSize == 4096);
    uint8_t cmd[4];
    cmd[0] = kCmdErase4K;

    while (length != 0) {
        watchdog::Feed();
        SpiFlashAddr(offset, cmd);

        SPI_FLASH_DEBUG_PRINTF("erase %2x %2x %2x %2x (%x)", cmd[0], cmd[1], cmd[2], cmd[3], static_cast<unsigned>(offset));

        const auto kRet = SpiFlashWriteCommon(cmd, sizeof(cmd), nullptr, 0, (length != spi::flash::kSectorSize));

        if (!kRet) {
            SPI_FLASH_DEBUG_PUTS("Erase failed");
            SPI_FLASH_DEBUG_EXIT();
            return false;
        }

        offset += spi::flash::kSectorSize;
        length -= spi::flash::kSectorSize;
    }

    SPI_FLASH_DEBUG_EXIT();
    return true;
}

bool WriteStatus(uint8_t status) {
    uint8_t cmd = kCmdWriteStatus;
    const auto kRet = SpiFlashWriteCommon(&cmd, 1, &status, 1, false);

    if (!kRet) {
        SPI_FLASH_DEBUG_PUTS("Fail to write status register");
        return false;
    }

    return true;
}
} // namespace cmd
} // namespace spi::flash
