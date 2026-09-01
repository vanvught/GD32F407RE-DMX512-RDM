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

static struct SpiFlashInfo s_flash = {.name = "", .size = 0, .poll_cmd = CMD_READ_STATUS};

#define IDCODE_PART_LEN 5

namespace {
constexpr struct {
    const uint8_t kIdcode;
    bool (*probe)(struct SpiFlashInfo* flash, const uint8_t* idcode);
} kFlashes[] = {
// Keep it sorted by define name
#ifdef CONFIG_SPI_FLASH_GIGADEVICE
    {
        .kIdcode = 0xc8,
        .probe = SpiFlashProbeGigadevice,
    },
#endif
#ifdef CONFIG_SPI_FLASH_MACRONIX
    {
        .kIdcode = 0xc2,
        .probe = SpiFlashProbeMacronix,
    },
#endif
#ifdef CONFIG_SPI_FLASH_WINBOND
    {
        .kIdcode = 0xef,
        .probe = SpiFlashProbeWinbond,
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

    SpiXfer(command_length, command, nullptr, flags);

    if (data_length != 0) {
        SpiXfer(data_length, data_out, data_in, SPI_XFER_END);
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
    SpiFlashCmd(CMD_WRITE_ENABLE, nullptr, 0);
}

bool SpiFlashCmdWaitReady(uint32_t timeout) {
    uint8_t cmd = CMD_READ_STATUS;

    SpiXfer(1, &cmd, nullptr, SPI_XFER_BEGIN);

    const auto kTimebase = GetTimer(0);
    uint8_t status;

    do {
        SpiXfer(1, nullptr, &status, 0);

        if ((status & STATUS_WIP) == 0) {
            break;
        }

    } while (GetTimer(kTimebase) < timeout);

    SpiXfer(0, nullptr, nullptr, SPI_XFER_END);

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
    uint32_t timeout;

    if (data == nullptr) {
        timeout = SPI_FLASH_PAGE_ERASE_TIMEOUT;
    } else {
        timeout = SPI_FLASH_PROG_TIMEOUT;
    }

    SpiFlashCmdWriteEnable();
    SpiFlashCmdWrite(command, command_length, data, data_length);

    if (wait_ready) {
        const auto kRet = SpiFlashCmdWaitReady(timeout);

        if (!kRet) {
            SPI_FLASH_DEBUG_PRINTF("write %s timed out", timeout == SPI_FLASH_PROG_TIMEOUT ? "program" : "page erase");
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
    SpiInit();

    uint8_t idcode[IDCODE_LEN];
    SpiFlashCmd(CMD_READ_ID, idcode, sizeof(idcode));

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
bool Read(uint32_t offset, uint32_t length, uint8_t* data) {
    SPI_FLASH_DEBUG_ENTRY();

    if (!SpiFlashCmdWaitReady(SPI_FLASH_PROG_TIMEOUT)) {
        SPI_FLASH_DEBUG_EXIT();
        return false;
    }

    uint8_t cmd[5];
    cmd[0] = CMD_READ_ARRAY_FAST;
    cmd[4] = 0x00;

    while (length != 0) {
        const auto kRemainLength = SPI_FLASH_16MB_BOUN - offset;
        uint32_t read_length;

        if (length < kRemainLength) {
            read_length = length;
        } else {
            read_length = kRemainLength;
        }

        SpiFlashAddr(offset, cmd);
        SpiFlashReadCommon(cmd, sizeof(cmd), data, read_length);

        offset += read_length;
        length -= read_length;
        data += read_length;
    }

    SPI_FLASH_DEBUG_EXIT();
    return true;
}

bool Write(uint32_t offset, uint32_t length, const uint8_t* data) {
    SPI_FLASH_DEBUG_ENTRY();

    if (!SpiFlashCmdWaitReady(SPI_FLASH_SECTOR_ERASE_TIMEOUT)) {
        SPI_FLASH_DEBUG_EXIT();
        return false;
    }

    uint32_t chunk_length;
    uint8_t cmd[4];
    cmd[0] = CMD_PAGE_PROGRAM;

    for (uint32_t actual_length = 0; actual_length < length; actual_length += chunk_length) {
        const auto kByteAddress = offset % spi::flash::kPageSize;
        chunk_length = common::Min((length - actual_length), (spi::flash::kPageSize - kByteAddress));

        SpiFlashAddr(offset, cmd);

        SPI_FLASH_DEBUG_PRINTF("0x%p => cmd = { 0x%02x 0x%02x%02x%02x } actual_length=%d, chunk_length=%d", data + actual_length, cmd[0], cmd[1], cmd[2], cmd[3], static_cast<int>(actual_length), static_cast<int>(chunk_length));

        const auto kRet = SpiFlashWriteCommon(cmd, sizeof(cmd), data + actual_length, chunk_length, ((actual_length + chunk_length) != length));

        if (!kRet) {
            SPI_FLASH_DEBUG_PUTS("write failed");
            SPI_FLASH_DEBUG_EXIT();
            return false;
            break;
        }

        offset += chunk_length;
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

    if (!SpiFlashCmdWaitReady(SPI_FLASH_PROG_TIMEOUT)) {
        SPI_FLASH_DEBUG_EXIT();
        return false;
    }

    static_assert(spi::flash::kSectorSize == 4096);
    uint8_t cmd[4];
    cmd[0] = CMD_ERASE_4K;

    while (length != 0) {
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
    uint8_t cmd = CMD_WRITE_STATUS;
    const auto kRet = SpiFlashWriteCommon(&cmd, 1, &status, 1, false);

    if (!kRet) {
        SPI_FLASH_DEBUG_PUTS("Fail to write status register");
        return false;
    }

    return true;
}
} // namespace cmd
} // namespace spi::flash
