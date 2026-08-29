/**
 * @file gigadevice.cpp
 *
 */
/*
 * Gigadevice SPI flash driver
 * Copyright 2013, Samsung Electronics Co., Ltd.
 * Author: Banajit Goswami <banajit.g@samsung.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
/*
 * Original code : https://github.com/martinezjavier/u-boot/blob/master/drivers/mtd/spi/gigadevice.c
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

#include "common/utils/utils_array.h"
#include "spi/spi_flash.h"
#include "spi_flash_internal.h"

struct GigadeviceSpiFlashParams {
    const uint16_t kId;
    const uint16_t kNrBlocks;
    const char* const kName;
};

static constexpr struct GigadeviceSpiFlashParams kGigadeviceSpiFlashTable[] = {
    {
        .kId = 0x6016,
        .kNrBlocks = 64,
        .kName = "GD25LQ",
    },
    {
        .kId = 0x4015,
        .kNrBlocks = 8,
        .kName = "GD25Q40",
    },
    {
        .kId = 0x4017,
        .kNrBlocks = 128,
        .kName = "GD25Q64B",
    },
};

bool SpiFlashProbeGigadevice(struct SpiFlashInfo* flash, const uint8_t* idcode) {
    SPI_FLASH_DEBUG_ENTRY();

    const struct GigadeviceSpiFlashParams* params;
    size_t index;

    for (index = 0; index < common::ArraySize(kGigadeviceSpiFlashTable); index++) {
        params = &kGigadeviceSpiFlashTable[index];
        if (params->kId == ((idcode[1] << 8) | idcode[2])) {
            break;
        }
    }

    if (index == common::ArraySize(kGigadeviceSpiFlashTable)) {
        SPI_FLASH_DEBUG_PRINTF("SF: Unsupported GigaDevice ID %02x%02x", idcode[1], idcode[2]);
        SPI_FLASH_DEBUG_EXIT();
        return false;
    }

    flash->name = params->kName;
    flash->size = 16U * spi::flash::kSectorSize * params->kNrBlocks;

    SPI_FLASH_DEBUG_EXIT();
    return true;
}
