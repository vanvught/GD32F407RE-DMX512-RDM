/**
 * @file macronix.cpp
 *
 */
/*
 * Copyright 2009(C) Marvell International Ltd. and its affiliates
 * Prafulla Wadaskar <prafulla@marvell.com>
 *
 * Based on drivers/mtd/spi/stmicro.c
 *
 * Copyright 2008, Network Appliance Inc.
 * Jason McMullan <mcmullan@netapp.com>
 *
 * Copyright (C) 2004-2007 Freescale Semiconductor, Inc.
 * TsiChung Liew (Tsi-Chung.Liew@freescale.com)
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
/*
 * Original code : https://github.com/martinezjavier/u-boot/blob/master/drivers/mtd/spi/macronix.c
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
#include "spi_flash_internal.h"
#include "common/utils/utils_array.h"

namespace {
struct MacronixSpiFlashParams {
    const uint16_t kIdcode;
    const uint16_t kNrBlocks;
    const char* const kName;
};

constexpr struct MacronixSpiFlashParams kMacronixSpiFlashTable[] = {
    {
        .kIdcode = 0x2013,
        .kNrBlocks = 8,
        .kName = "MX25L4005",
    },
    {
        .kIdcode = 0x2014,
        .kNrBlocks = 16,
        .kName = "MX25L8005",
    },
    {
        .kIdcode = 0x2015,
        .kNrBlocks = 32,
        .kName = "MX25L1605D",
    },
    {
        .kIdcode = 0x2016,
        .kNrBlocks = 64,
        .kName = "MX25L3205D",
    },
    {
        .kIdcode = 0x2017,
        .kNrBlocks = 128,
        .kName = "MX25L6405D",
    },
    {
        .kIdcode = 0x2018,
        .kNrBlocks = 256,
        .kName = "MX25L12805D",
    },
    {
        .kIdcode = 0x2618,
        .kNrBlocks = 256,
        .kName = "MX25L12855E",
    },
};
} // namespace

namespace spi::flash {
bool ProbeMacronix(struct spi::flash::Info* flash, const uint8_t* idcode) {
    SPI_FLASH_DEBUG_ENTRY();

    const struct MacronixSpiFlashParams* params;
    size_t index;
    uint32_t id_code = idcode[2] | static_cast<uint32_t>(idcode[1] << 8);

    for (index = 0; index < common::ArraySize(kMacronixSpiFlashTable); index++) {
        params = &kMacronixSpiFlashTable[index];

        if (params->kIdcode == id_code) {
            break;
        }
    }

    if (index == common::ArraySize(kMacronixSpiFlashTable)) {
        SPI_FLASH_DEBUG_PRINTF("Unsupported Macronix ID %04x\n", static_cast<unsigned>(id_code));
        SPI_FLASH_DEBUG_EXIT();
        return false;
    }

    flash->name = params->kName;
    flash->size = 16U * spi::flash::kSectorSize * params->kNrBlocks;

    // Clear BP# bits for read-only flash
    spi::flash::cmd::WriteStatus(0);

    SPI_FLASH_DEBUG_EXIT();
    return true;
}
} // namespace spi::flash
