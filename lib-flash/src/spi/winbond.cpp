/**
 * @file winbond.cpp
 *
 */
/*
 * Copyright 2008, Network Appliance Inc.
 * Author: Jason McMullan <mcmullan <at> netapp.com>
 * Licensed under the GPL-2 or later.
 */
/*
 * Original code : https://github.com/martinezjavier/u-boot/blob/master/drivers/mtd/spi/winbond.c
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

#include <cstddef>
#include <cstdint>

#include "common/utils/utils_array.h"
#include "spi/spi_flash.h"
#include "spi_flash_internal.h"

struct WinbondSpiFlashParams
{
    const uint16_t kId;
    const uint16_t kNrBlocks;
    const char* const kName;
};

static constexpr struct WinbondSpiFlashParams kWinbondSpiFlashTable[] = {
  {
         .kId = 0x3013,
         .kNrBlocks = 8,
         .kName = "W25X40",
     },
     {
         .kId = 0x3015,
         .kNrBlocks = 32,
         .kName = "W25X16",
     },
     {
         .kId = 0x3016,
         .kNrBlocks = 64,
         .kName = "W25X32",
     },
     {
         .kId = 0x3017,
         .kNrBlocks = 128,
         .kName = "W25X64",
     },
     {
         .kId = 0x4014,
         .kNrBlocks = 16,
         .kName = "W25Q80BL",
     },
     {
         .kId = 0x4015,
         .kNrBlocks = 32,
         .kName = "W25Q16CL",
     },
     {
         .kId = 0x4016,
         .kNrBlocks = 64,
         .kName = "W25Q32BV",
     },
     {
         .kId = 0x4017,
         .kNrBlocks = 128,
         .kName = "W25Q64CV",
     },
     {
         .kId = 0x4018,
         .kNrBlocks = 256,
         .kName = "W25Q128BV",
     },
     {
         .kId = 0x4019,
         .kNrBlocks = 512,
         .kName = "W25Q256",
     },
     {
         .kId = 0x5014,
         .kNrBlocks = 16,
         .kName = "W25Q80BW",
     },
     {
         .kId = 0x6015,
         .kNrBlocks = 32,
         .kName = "W25Q16DW",
     }};

bool SpiFlashProbeWinbond(struct SpiFlashInfo* flash, const uint8_t* idcode) {
    SPI_FLASH_DEBUG_ENTRY();

    const struct WinbondSpiFlashParams* params;
    size_t index;

    for (index = 0; index < common::ArraySize(kWinbondSpiFlashTable); index++) {
        params = &kWinbondSpiFlashTable[index];
        if (params->kId == ((idcode[1] << 8) | idcode[2])) {
            break;
        }
    }

    if (index == common::ArraySize(kWinbondSpiFlashTable)) {
        SPI_FLASH_DEBUG_PRINTF("Unsupported Winbond ID %02x%02x", idcode[1], idcode[2]);
        SPI_FLASH_DEBUG_EXIT();
        return false;
    }

    flash->name = params->kName;
    flash->size = 16U * spi::flash::kSectorSize * params->kNrBlocks;

    SPI_FLASH_DEBUG_EXIT();
    return true;
}
